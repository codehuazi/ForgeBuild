#pragma once

#include <string>
#include <unordered_map>
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

    const std::vector<std::string>& reasons(
        const Edge* edge
    ) const;

private:


    void refresh_nodes();

    std::vector<Edge*> collect_edges_to_build();


    void append_file_state_reasons(
        const Edge* edge,
        std::vector<std::string>& reasons
    ) const;


    void append_command_change_reasons(
        const Edge* edge,
        std::vector<std::string>& reasons
    ) const;


    void append_dynamic_dependency_reasons(
        const Edge* edge,
        std::vector<std::string>& reasons
    ) const;



private:

    BuildGraph& graph_;

    const BuildLog* build_log_;

    const DepsLog* deps_log_;

    std::unordered_map<
        const Edge*,
        std::vector<std::string>
    > reasons_;

};


}