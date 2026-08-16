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
#include <stdexcept>


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
    /*
     * 没有指定 Target 时保持原有行为：
     * 对整个 BuildGraph 做 Dirty Analysis。
     */

    refresh_nodes();


    auto edges =
        collect_edges_to_build(
            nullptr
        );


    return make_plan(
        edges
    );
}


BuildPlan Builder::build(
    const std::vector<std::string>& targets
)
{
    /*
     * 空 Target 集合等价于完整构建。
     */
    if(targets.empty())
    {
        return build();
    }


    /*
     * 先计算用户真正要求的 Target
     * 所对应的上游 Edge 闭包。
     *
     * 然后 Dirty Analysis 只允许在这个闭包内进行。
     */
    const auto target_closure =
        collect_target_closure(
            targets
        );


    refresh_nodes();


    auto edges =
        collect_edges_to_build(
            &target_closure
        );


    return make_plan(
        edges
    );
}


BuildPlan Builder::make_plan(
    const std::vector<Edge*>& edges
)
{
    BuildPlanner planner;


    auto ordered =
        planner.plan(
            edges
        );


    BuildPlan plan;


    for(auto* edge :
        ordered)
    {
        plan.add_edge(
            edge
        );
    }


    return plan;
}


std::unordered_set<Edge*>
Builder::collect_target_closure(
    const std::vector<std::string>& targets
) const
{
    std::unordered_set<Edge*>
        closure;


    std::vector<Edge*>
        pending_edges;


    /*
     * Target 是 BuildGraph 中的 Output Node。
     *
     * Node::in_edge() 指向生成这个 Node 的 Producer Edge。
     */
    for(const std::string& target :
        targets)
    {
        const auto iterator =
            graph_.nodes().find(
                target
            );


        if(iterator
            == graph_.nodes().end())
        {
            throw std::invalid_argument(
                "unknown target: "
                + target
            );
        }


        Edge* producer =
            iterator->second
                ->in_edge();


        if(producer == nullptr)
        {
            throw std::invalid_argument(
                "target is not produced by any build edge: "
                + target
            );
        }


        pending_edges.push_back(
            producer
        );
    }


    /*
     * 从 Target Producer 开始反向遍历。
     *
     * Edge
     *   ↑
     * input Node
     *   ↑
     * input->in_edge()
     *
     * 最终得到所有必须位于 Target 上游的 Producer Edge。
     */
    while(!pending_edges.empty())
    {
        Edge* edge =
            pending_edges.back();


        pending_edges.pop_back();


        const bool inserted =
            closure.insert(
                edge
            ).second;


        if(!inserted)
        {
            continue;
        }


        for(Node* input :
            edge->inputs())
        {
            Edge* producer =
                input->in_edge();


            if(producer != nullptr)
            {
                pending_edges.push_back(
                    producer
                );
            }
        }
    }


    return closure;
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
        /*
         * 显式输入不存在和“输入时间戳较旧”
         * 是完全不同的状态。
         *
         * 输入已经不存在时，不能再依赖 timestamp
         * 判断是否需要重建。
         */
        if (!input->exists())
        {
            reasons.push_back(
                "input missing: "
                + input->path()
            );

            continue;
        }


        for (const Node* output :
             edge->outputs())
        {
            /*
             * Output 已经缺失时，上面已经记录了
             * "output missing"。
             *
             * 此时没有必要再追加
             * "input newer than output"，避免 --explain
             * 输出重复、无意义的原因。
             */
            if (!output->exists())
            {
                continue;
            }


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


std::vector<Edge*> Builder::collect_edges_to_build(
    const std::unordered_set<Edge*>*
        allowed_edges
)
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

        if(allowed_edges != nullptr
            && !allowed_edges->contains(
                edge
            ))
        {
            continue;
        }

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
                /*
                * Target Build 中，只允许 Dirty 状态在
                * 当前 Target 的上游闭包内部传播。
                *
                * 即使同一个 Output 还有其他下游消费者，
                * 只要它不属于当前 Target，就不能进入 BuildPlan。
                */
                if(allowed_edges != nullptr
                    && !allowed_edges->contains(
                        dependent_edge
                    ))
                {
                    continue;
                }


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