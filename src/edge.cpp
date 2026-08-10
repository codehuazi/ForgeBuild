#include "forge/edge.hpp"

#include "forge/node.hpp"
#include "forge/rule.hpp"
#include "forge/string_utils.hpp"

#include <sstream>

namespace forge {

Edge::Edge(Rule* rule):rule_(rule)
{

}

Rule* Edge::rule() const
{
    return rule_;
}

bool Edge::needs_build() const
{
    for (const auto* output : outputs_)
    {
        if (!output->exists())
        {
            return true;
        }
    }

    for (const auto* input : inputs_)
    {
        for (const auto* output : outputs_)
        {
            if (input->timestamp() > output->timestamp())
            {
                return true;
            }
        }
    }

    return false;
}

std::string Edge::describe() const
{

    std::ostringstream oss;


    for(size_t i = 0; i < inputs_.size(); i++)
    {
        if(i != 0)
        {
            oss << " + ";
        }

        oss << inputs_[i]->path();
    }


    oss
        << " --"
        << rule_->name()
        << "--> ";


    for(size_t i = 0; i < outputs_.size(); i++)
    {
        if(i != 0)
        {
            oss << " + ";
        }

        oss << outputs_[i]->path();
    }


    return oss.str();

}

std::string Edge::command() const
{

    std::string cmd =
        rule_->command();


    std::string input;


    for(size_t i=0;i<inputs_.size();i++)
    {

        if(i!=0)
        {
            input += " ";
        }


        input += inputs_[i]->path();

    }



    std::string output;


    for(size_t i=0;i<outputs_.size();i++)
    {

        if(i!=0)
        {
            output += " ";
        }


        output += outputs_[i]->path();

    }



    replace_all(
        cmd,
        "$in",
        input
    );


    replace_all(
        cmd,
        "$out",
        output
    );


    return cmd;

}

std::string Edge::depfile() const
{
    std::string path =
        rule_->depfile();


    if (path.empty())
    {
        return {};
    }


    std::string input;


    for (std::size_t i = 0;
         i < inputs_.size();
         ++i)
    {
        if (i != 0)
        {
            input += " ";
        }


        input +=
            inputs_[i]->path();
    }


    std::string output;


    for (std::size_t i = 0;
         i < outputs_.size();
         ++i)
    {
        if (i != 0)
        {
            output += " ";
        }


        output +=
            outputs_[i]->path();
    }


    replace_all(
        path,
        "$in",
        input
    );


    replace_all(
        path,
        "$out",
        output
    );


    return path;
}

bool Edge::depends_on(
    const Edge* other
) const
{
    for(auto* input : inputs_)
    {
        for(auto* output : other->outputs_)
        {
            if(input == output)
            {
                return true;
            }
        }
    }


    return false;
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