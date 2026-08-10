#pragma once

#include "forge/build_graph.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace forge {

class Rule;

class Manifest
{
public:

    Manifest();

    ~Manifest();

    Rule* add_rule(
        std::string name,
        std::string command
    );

    Rule* find_rule(
        const std::string& name
    ) const;

    BuildGraph& graph();

    const BuildGraph& graph() const;

private:
    std::unordered_map<
        std::string,
        std::unique_ptr<Rule>
    > rules_;

    BuildGraph graph_;
};

} // namespace forge