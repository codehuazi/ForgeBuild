#include "forge/node.hpp"

#include <utility>

namespace forge {

Node::Node(std::string path)
    : path_(std::move(path))
{
}

const std::string& Node::path() const
{
    return path_;
}

Edge* Node::in_edge() const
{
    return in_edge_;
}

void Node::set_in_edge(Edge* edge)
{
    in_edge_ = edge;
}

const std::vector<Edge*>& Node::out_edges() const
{
    return out_edges_;
}

void Node::add_out_edge(Edge* edge)
{
    out_edges_.push_back(edge);
}

} // namespace forge