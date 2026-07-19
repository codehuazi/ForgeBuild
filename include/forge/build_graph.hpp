#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace forge {

class Node;
class Edge;
class Rule;

class BuildGraph
{
public:
    BuildGraph() = default;

    ~BuildGraph();

    BuildGraph(const BuildGraph&) = delete;
    BuildGraph& operator=(const BuildGraph&) = delete;

    Node* get_or_create_node(const std::string& path);

    Edge* create_edge(Rule* rule);

    void add_input(Edge* edge, Node* node);

    void add_output(Edge* edge, Node* node);

private:
    std::unordered_map<
        std::string,
        std::unique_ptr<Node>
    > nodes_;

    std::vector<
        std::unique_ptr<Edge>
    > edges_;
};

} // namespace forge