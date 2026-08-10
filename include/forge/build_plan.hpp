#pragma once

#include <vector>


namespace forge
{

class Edge;


class BuildPlan
{

public:


    void add_edge(
        Edge* edge
    );


    const std::vector<Edge*>& edges() const;

    void print() const;


private:

    std::vector<Edge*> edges_;

};


}