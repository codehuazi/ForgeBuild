#pragma once

#include <vector>


namespace forge {


class Node;
class Rule;


class Edge
{

public:

    explicit Edge(Rule* rule);


    void add_input(Node* node);


    void add_output(Node* node);


    const std::vector<Node*>& inputs() const;


    const std::vector<Node*>& outputs() const;


private:

    Rule* rule_;


    std::vector<Node*> inputs_;


    std::vector<Node*> outputs_;

};


} // namespace forge