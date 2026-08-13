#include "forge/builder.hpp"

#include "forge/build_graph.hpp"
#include "forge/build_plan.hpp"
#include "forge/build_planner.hpp"
#include "forge/build_log.hpp"
#include "forge/hash.hpp"
#include "forge/edge.hpp"
#include "forge/node.hpp"
#include "forge/rule.hpp"
#include "forge/deps_log.hpp"
#include "forge/file_system.hpp"


#include <queue>
#include <unordered_set>
#include <vector>


namespace forge
{


Builder::Builder(
    BuildGraph& graph
)
    :
    graph_(graph),
    build_log_(nullptr),
    deps_log_(nullptr)
{
}


Builder::Builder(
    BuildGraph& graph,
    const BuildLog& build_log
)
    :
    graph_(graph),
    build_log_(&build_log),
    deps_log_(nullptr)
{
}


Builder::Builder(
    BuildGraph& graph,
    const BuildLog& build_log,
    const DepsLog& deps_log
)
    :
    graph_(graph),
    build_log_(&build_log),
    deps_log_(&deps_log)
{
}

BuildPlan Builder::build()
{
    // 第一步：从磁盘刷新所有节点状态。
    refresh_nodes();


    // 第二步：找出本次需要执行的所有 Edge。
    //
    // 这里不仅包含：
    // 1. 输出不存在的 Edge
    // 2. 输入比输出新的 Edge
    //
    // 还会包含：
    // 3. 因上游输出即将变化而需要执行的下游 Edge
    auto edges =
        collect_edges_to_build();


    // 第三步：按照依赖关系进行拓扑排序。
    BuildPlanner planner;

    auto ordered =
        planner.plan(edges);


    // 第四步：把排序后的 Edge 写入 BuildPlan。
    BuildPlan plan;

    for(auto* edge : ordered)
    {
        plan.add_edge(edge);
    }


    return plan;
}


const std::vector<std::string>& Builder::reasons(
    const Edge* edge
) const
{
    static const std::vector<std::string>
        empty_reasons;


    const auto iterator =
        reasons_.find(
            edge
        );


    if (iterator
        == reasons_.end())
    {
        return empty_reasons;
    }


    return iterator->second;
}


void Builder::refresh_nodes()
{
    for (const auto& entry : graph_.nodes())
    {
        entry.second->refresh();
    }
}


void Builder::append_file_state_reasons(
    const Edge* edge,
    std::vector<std::string>& reasons
) const
{
    for (const Node* output :
         edge->outputs())
    {
        if (!output->exists())
        {
            reasons.push_back(
                "output missing: "
                + output->path()
            );
        }
    }


    for (const Node* input :
         edge->inputs())
    {
        for (const Node* output :
             edge->outputs())
        {
            if (input->timestamp()
                > output->timestamp())
            {
                reasons.push_back(
                    "input newer than output: "
                    + input->path()
                    + " -> "
                    + output->path()
                );
            }
        }
    }
}


void Builder::append_command_change_reasons(
    const Edge* edge,
    std::vector<std::string>& reasons
) const
{
    if (build_log_ == nullptr)
    {
        return;
    }


    const std::uint64_t current_hash =
        hash_string(
            edge->command()
        );


    for (const Node* output :
         edge->outputs())
    {
        if (!build_log_->command_matches(
                output->path(),
                current_hash
            ))
        {
            reasons.push_back(
                "command changed for output: "
                + output->path()
            );
        }
    }
}


void Builder::append_dynamic_dependency_reasons(
    const Edge* edge,
    std::vector<std::string>& reasons
) const
{
    if (deps_log_ == nullptr)
    {
        return;
    }


    for (const Node* output :
         edge->outputs())
    {
        const std::string& output_path =
            output->path();


        if (!deps_log_->contains(
                output_path
            ))
        {
            continue;
        }


        const auto& dependencies =
            deps_log_->inputs(
                output_path
            );

            
                for (const std::string& dependency :
             dependencies)
        {
            bool is_explicit_input = false;


            for (const Node* input :
                 edge->inputs())
            {
                if (input->path()
                    == dependency)
                {
                    is_explicit_input = true;

                    break;
                }
            }


            if (is_explicit_input)
            {
                continue;
            }


            if (!FileSystem::exists(
                    dependency
                ))
            {
                reasons.push_back(
                    "dynamic dependency missing: "
                    + dependency
                    + " -> "
                    + output_path
                );

                continue;
            }


            if (FileSystem::timestamp(
                    dependency
                )
                > output->timestamp())
            {
                reasons.push_back(
                    "dynamic dependency newer than output: "
                    + dependency
                    + " -> "
                    + output_path
                );
            }
        }
    }
}


std::vector<Edge*> Builder::collect_edges_to_build()
{
    std::vector<Edge*> result;

    std::queue<Edge*> pending_edges;

    std::unordered_set<Edge*> queued_edges;

    reasons_.clear();


    // 第一阶段：找到因为真实磁盘状态而直接过期的 Edge。
    for (const auto& edge_owner : graph_.edges())
    {
        Edge* edge =
            edge_owner.get();

        std::vector<std::string>
            edge_reasons;


        append_file_state_reasons(
            edge,
            edge_reasons
        );


        append_command_change_reasons(
            edge,
            edge_reasons
        );


        append_dynamic_dependency_reasons(
            edge,
            edge_reasons
        );


        if (edge_reasons.empty())
        {
            continue;
        }


        reasons_.emplace(
            edge,
            std::move(
                edge_reasons
            )
        );


        pending_edges.push(
            edge
        );


        queued_edges.insert(
            edge
        );
    }


    // 第二阶段：从直接过期的 Edge 向下游传播。
    while (!pending_edges.empty())
    {
        Edge* edge =
            pending_edges.front();

        pending_edges.pop();


        result.push_back(edge);


        for (Node* output : edge->outputs())
        {
            output->mark_dirty();


            for (Edge* dependent_edge :
                 output->out_edges())
            {
                reasons_[dependent_edge]
                    .push_back(
                        "upstream output is dirty: "
                        + output->path()
                    );


                const bool inserted =
                    queued_edges
                        .insert(
                            dependent_edge
                        )
                        .second;


                if (inserted)
                {
                    pending_edges.push(
                        dependent_edge
                    );
                }
            }
        }
    }


    return result;
}

}