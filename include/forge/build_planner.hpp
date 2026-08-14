#pragma once

#include <vector>


namespace forge
{


class Edge;


class BuildPlanner
{

public:

    std::vector<Edge*> plan(
        const std::vector<Edge*>& edges
    );

};


}