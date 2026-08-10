#include "forge/build_graph.hpp"

#include "forge/node.hpp"
#include "forge/edge.hpp"
#include "forge/rule.hpp"

#include <iostream>

#include <stdexcept>

namespace forge {

BuildGraph::BuildGraph() = default;

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

void BuildGraph::dump() const
{

    std::cout
        << "===== Build Graph =====\n";


    std::cout
        << "\nNodes:\n";


    for(auto& [path,node] : nodes_)
    {

        std::cout
            << "  "
            << path
            << "\n";

    }


    std::cout
        << "\nEdges:\n";


    for(auto& edge_ptr : edges_)
    {

        auto* edge =
            edge_ptr.get();


        std::cout
            << "Edge\n";


        std::cout
            << " Rule: "
            << edge->rule()->name()
            << "\n";


        std::cout
            << " Inputs:\n";


        for(auto* node :
            edge->inputs())
        {

            std::cout
                << "   "
                << node->path()
                << "\n";

        }


        std::cout
            << " Outputs:\n";


        for(auto* node :
            edge->outputs())
        {

            std::cout
                << "   "
                << node->path()
                << "\n";

        }

    }


}

const std::vector<
    std::unique_ptr<Edge>
>& BuildGraph::edges() const
{
    return edges_;
}

const std::unordered_map<
    std::string,
    std::unique_ptr<Node>
>& BuildGraph::nodes() const
{
    return nodes_;
}

}