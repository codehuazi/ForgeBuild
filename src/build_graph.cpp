#include "forge/build_graph.hpp"

#include "forge/node.hpp"
#include "forge/edge.hpp"

#include <stdexcept>

namespace forge {

BuildGraph::~BuildGraph() = default;

Node* BuildGraph::get_or_create_node(
    const std::string& path
)
{

    auto it = nodes_.find(path);

    if(it != nodes_.end())
    {
        return it->second.get();
    }

    auto node =
        std::make_unique<Node>(path);

    Node* ptr = node.get();

    nodes_.emplace(
        path,
        std::move(node)
    );

    return ptr;

}

Edge* BuildGraph::create_edge(
    Rule* rule
)
{
    auto edge =
        std::make_unique<Edge>(rule);

    Edge* ptr=edge.get();

    edges_.push_back(
        std::move(edge)
    );
    
    return ptr;

}

void BuildGraph::add_input(Edge* edge, Node* node)
{
    edge->add_input(node);
    node->add_out_edge(edge);
}

void BuildGraph::add_output(Edge* edge, Node* node)
{
    if (node->in_edge() != nullptr) {
        throw std::runtime_error(
            "node already has a producer: " + node->path()
        );
    }
    
    edge->add_output(node);
    node->set_in_edge(edge);
}

}