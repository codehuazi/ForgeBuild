#include "forge/build_graph_validator.hpp"

#include "forge/build_graph.hpp"
#include "forge/build_planner.hpp"
#include "forge/edge.hpp"

#include <vector>


namespace forge
{


void BuildGraphValidator::validate(
    const BuildGraph& graph
) const
{
    std::vector<Edge*> all_edges;


    all_edges.reserve(
        graph.edges().size()
    );


    for(const auto& edge :
        graph.edges())
    {
        all_edges.push_back(
            edge.get()
        );
    }


    /*
     * BuildPlanner 已经实现任务 DAG 的拓扑排序以及
     * 环检测，这里直接复用，避免维护第二套算法。
     */
    BuildPlanner planner;


    static_cast<void>(
        planner.plan(
            all_edges
        )
    );
}


}