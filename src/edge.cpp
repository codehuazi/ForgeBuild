#include "forge/edge.hpp"

namespace forge {

Edge::Edge(Rule* rule):rule_(rule)
{

}

void Edge::add_input(Node* node)
{
    inputs_.push_back(node);
}

void Edge::add_output(Node* node)
{
    outputs_.push_back(node);
}

const std::vector<Node*>& Edge::inputs() const
{
    return inputs_;
}

const std::vector<Node*>& Edge::outputs() const
{
    return outputs_;
}

}// namespace forge