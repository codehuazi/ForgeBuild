#include "forge/node.hpp"
#include "forge/file_system.hpp"

#include <utility>

namespace forge {

Node::Node(std::string path)
    :
    path_(std::move(path)),
    dirty_(false),
    exists_(false),
    timestamp_(0)
{
}

const std::string& Node::path() const
{
    return path_;
}

bool Node::exists() const
{
    return exists_;
}


bool Node::dirty() const
{
    return dirty_;
}

long long Node::timestamp() const
{
    return timestamp_;
}

void Node::set_dirty(
    bool dirty
)
{
    dirty_ = dirty;
}

void Node::refresh()
{
    exists_ =
        FileSystem::exists(path_);

    timestamp_ =
        FileSystem::timestamp(path_);
}

void Node::mark_dirty()
{
    dirty_ = true;
}

void Node::mark_clean()
{
    dirty_ = false;
}

void Node::mark_exists()
{
    exists_ = true;
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