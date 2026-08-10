#include "forge/build_planner.hpp"

#include "forge/edge.hpp"
#include "forge/node.hpp"


#include <unordered_map>
#include <queue>
#include <iostream>
#include <stdexcept>


namespace forge
{

std::vector<Edge*> BuildPlanner::plan(
    const std::vector<Edge*>& edges
)
{

    std::vector<Edge*> result;


    std::unordered_map<Edge*, int> indegree;



    for(auto* edge : edges)
    {
        indegree[edge] = 0;
    }

    for(auto* edge : edges)
    {

        for(auto* input : edge->inputs())
        {

            auto* producer =
                input->in_edge();


            if(producer != nullptr)
            {

                if(indegree.contains(producer))
                {
                    indegree[edge]++;
                }

            }

        }

    }

    std::queue<Edge*> queue;


    for(auto& [edge, degree] : indegree)
    {
        if(degree == 0)
        {
            queue.push(edge);
        }
    }

    while(!queue.empty())
    {

        Edge* current =
            queue.front();


        queue.pop();


        result.push_back(current);



        for(auto* edge : edges)
        {

            if(indegree[edge] == 0)
            {
                continue;
            }


            if(edge->depends_on(current))
            {

                indegree[edge]--;


                if(indegree[edge] == 0)
                {
                    queue.push(edge);
                }

            }

        }

    }

    if(result.size() != edges.size())
    {
        throw std::runtime_error(
            "cycle dependency detected"
        );
    }

    return result;

}


}