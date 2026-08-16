#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
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


    /*
     * 构建整个 Manifest 中所有需要执行的 Edge。
     */
    BuildPlan build();


    /*
     * 只构建指定 Target 及其上游依赖闭包中
     * 真正需要重新执行的 Edge。
     */
    BuildPlan build(
        const std::vector<std::string>& targets
    );


    const std::vector<std::string>& reasons(
        const Edge* edge
    ) const;


private:

    void refresh_nodes();


    BuildPlan make_plan(
        const std::vector<Edge*>& edges
    );


    std::unordered_set<Edge*>
    collect_target_closure(
        const std::vector<std::string>& targets
    ) const;


    std::vector<Edge*> collect_edges_to_build(
        const std::unordered_set<Edge*>*
            allowed_edges
    );


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