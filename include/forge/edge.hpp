#pragma once

#include <vector>

#include <string>


namespace forge {


class Node;
class Rule;


class Edge
{

public:

    explicit Edge(Rule* rule);

    Rule* rule() const;

    bool needs_build() const;

    std::string describe() const;

    std::string command() const;

    std::string depfile() const;

    bool depends_on(
        const Edge* other
    ) const;

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