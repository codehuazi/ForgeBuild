#include "forge/build_plan.hpp"

#include "forge/edge.hpp"

#include <iostream>


namespace forge
{


void BuildPlan::add_edge(
    Edge* edge
)
{
    edges_.push_back(edge);
}



const std::vector<Edge*>&
BuildPlan::edges() const
{
    return edges_;
}

void BuildPlan::print() const
{

    std::cout
        << "===== Build Plan =====\n";


    for(auto* edge : edges_)
    {
        std::cout
            << edge->describe()
            << "\n";
    }


    std::cout
        << "======================\n";

}


}