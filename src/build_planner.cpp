#include "forge/build_planner.hpp"

#include "forge/edge.hpp"
#include "forge/node.hpp"

#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace forge
{


std::vector<Edge*> BuildPlanner::plan(
    const std::vector<Edge*>& edges
)
{
    std::vector<Edge*> result;

    result.reserve(
        edges.size()
    );


    //
    // indegree：
    //
    // 当前 BuildPlan 中，每个 Edge 尚未完成的
    // producer Edge 数量。
    //
    std::unordered_map<
        Edge*,
        std::size_t
    > indegree;


    //
    // dependents：
    //
    // producer -> 直接依赖它的 consumer Edge。
    //
    // 预先建立邻接表后，Kahn 拓扑排序过程中
    // 不需要每弹出一个 Edge 就重新扫描整张图。
    //
    std::unordered_map<
        Edge*,
        std::vector<Edge*>
    > dependents;


    for(auto* edge : edges)
    {
        indegree.emplace(
            edge,
            0
        );

        dependents.emplace(
            edge,
            std::vector<Edge*>{}
        );
    }


    //
    // 一个 consumer 可能同时使用同一个 producer
    // 生成的多个 Output。
    //
    // 例如：
    //
    //   producer -> out1 -> consumer
    //            -> out2 -> consumer
    //
    // 从任务 DAG 的角度看，consumer 对 producer
    // 只有一条任务级依赖，而不是两条。
    //
    for(auto* edge : edges)
    {
        std::unordered_set<Edge*>
            unique_producers;


        for(auto* input : edge->inputs())
        {
            Edge* producer =
                input->in_edge();


            if(producer == nullptr)
            {
                continue;
            }


            //
            // 只考虑当前 BuildPlan 中的 producer。
            //
            // 如果 producer 不在本次计划中，
            // 表示它的 Output 可以直接复用，
            // 不应该阻塞当前 Edge。
            //
            if(!indegree.contains(
                producer
            ))
            {
                continue;
            }


            unique_producers.insert(
                producer
            );
        }


        indegree[edge] =
            unique_producers.size();


        for(auto* producer :
            unique_producers)
        {
            dependents[producer]
                .push_back(
                    edge
                );
        }
    }


    std::queue<Edge*> ready_queue;


    //
    // Kahn：
    // 所有入度为 0 的 Edge 初始可执行。
    //
    for(auto* edge : edges)
    {
        if(indegree[edge] == 0)
        {
            ready_queue.push(
                edge
            );
        }
    }


    while(!ready_queue.empty())
    {
        Edge* current =
            ready_queue.front();

        ready_queue.pop();


        result.push_back(
            current
        );


        //
        // 只访问 current 的直接 dependent，
        // 不再扫描所有 Edge。
        //
        for(auto* dependent :
            dependents[current])
        {
            auto& degree =
                indegree[dependent];


            if(degree == 0)
            {
                throw std::logic_error(
                    "invalid build planner indegree state"
                );
            }


            --degree;


            if(degree == 0)
            {
                ready_queue.push(
                    dependent
                );
            }
        }
    }


    //
    // Kahn 算法结束后如果仍有 Edge
    // 没有进入结果，说明存在真正的环。
    //
    if(result.size() != edges.size())
    {
        throw std::runtime_error(
            "cycle dependency detected"
        );
    }


    return result;
}


}