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


void Builder::refresh_nodes()
{
    for (const auto& entry : graph_.nodes())
    {
        entry.second->refresh();
    }
}

bool Builder::command_changed(
    const Edge* edge
) const
{
    if (build_log_ == nullptr)
    {
        return false;
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
            return true;
        }
    }


    return false;
}


bool Builder::dynamic_dependency_changed(
    const Edge* edge
) const
{
    if (deps_log_ == nullptr)
    {
        return false;
    }

    for (const Node* output : edge->outputs())
    {
        const std::string& output_path =
            output->path();

        if (!deps_log_->contains(output_path))
        {
            continue;
        }

        const auto& dependencies =
            deps_log_->inputs(output_path);

        for (const std::string& dependency :
             dependencies)
        {
            if (!FileSystem::exists(dependency))
            {
                return true;
            }

            if (FileSystem::timestamp(dependency)
                > output->timestamp())
            {
                return true;
            }
        }
    }

    return false;
}

std::vector<Edge*> Builder::collect_edges_to_build()
{
    std::vector<Edge*> result;

    std::queue<Edge*> pending_edges;

    std::unordered_set<Edge*> queued_edges;


    // 第一阶段：找到因为真实磁盘状态而直接过期的 Edge。
    for (const auto& edge_owner : graph_.edges())
    {
        Edge* edge =
            edge_owner.get();

        const bool file_state_requires_build =
            edge->needs_build();

        const bool command_requires_build =
            command_changed(edge);

        const bool dependency_requires_build =
            dynamic_dependency_changed(edge);


        if (!file_state_requires_build
            && !command_requires_build
            && !dependency_requires_build)
        {
            continue;
        }


        pending_edges.push(edge);

        queued_edges.insert(edge);
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


            for (Edge* dependent_edge : output->out_edges())
            {
                bool inserted =
                    queued_edges
                        .insert(dependent_edge)
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