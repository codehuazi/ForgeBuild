#pragma once

#include <vector>

#include "forge/build_plan.hpp"
#include "forge/build_planner.hpp"

namespace forge
{


class BuildGraph;

class BuildLog;

class Edge;

class DepsLog;


class Builder
{

public:

    explicit Builder(
        BuildGraph& graph
    );

    Builder(
        BuildGraph& graph,
        const BuildLog& build_log
    );

    Builder(
        BuildGraph& graph,
        const BuildLog& build_log,
        const DepsLog& deps_log
    );


    BuildPlan build();

private:


    void refresh_nodes();

    std::vector<Edge*> collect_edges_to_build();

    bool command_changed(
        const Edge* edge
    ) const;

    bool dynamic_dependency_changed(
        const Edge* edge
    ) const;



private:

    BuildGraph& graph_;

    const BuildLog* build_log_;

    const DepsLog* deps_log_;

};


}