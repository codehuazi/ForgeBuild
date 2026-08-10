#include "forge/parser.hpp"

#include "forge/manifest.hpp"
#include "forge/string_utils.hpp"
#include "forge/rule.hpp"

#include "forge/build_graph.hpp"
#include "forge/edge.hpp"
#include "forge/node.hpp"


#include <fstream>
#include <iostream>


namespace forge {


bool Parser::parse_file(
    const std::string& path,
    Manifest& manifest
)
{
    std::ifstream file(path);


    if(!file.is_open())
    {
        std::cerr
            << "cannot open file: "
            << path
            << '\n';

        return false;
    }


    std::string line;


    while(std::getline(file, line))
    {
        if(!parse_line(
            line,
            manifest
        ))
        {
            return false;
        }
    }


    return true;
}



bool Parser::parse_line(
    const std::string& raw,
    Manifest& manifest
)
{

    std::string line =
        trim(raw);



    if(line.empty())
    {
        return true;
    }


    if(starts_with(line,"rule "))
    {

        std::string name =
            line.substr(5);


        current_rule_ =
            manifest.add_rule(
                name,
                ""
            );

        state_ =
            State::Rule;

    }
    else if(starts_with(line,"command ="))
    {

        if(state_ != State::Rule)
        {
            return false;
        }


        std::string command =
            trim(
                line.substr(9)
            );


        current_rule_->set_command(
            command
        );

    }
    else if(starts_with(line, "depfile ="))
    {
        if(state_ != State::Rule
            || current_rule_ == nullptr)
        {
            return false;
        }


        std::string depfile =
            trim(
                line.substr(9)
            );


        if(depfile.empty())
        {
            return false;
        }


        current_rule_->set_depfile(
            depfile
        );
    }
    else if(starts_with(line,"build "))
    {
        return parse_build(
            line,
            manifest
        );
    }

    return true;
}

bool Parser::parse_build(
    const std::string& line,
    Manifest& manifest
)
{

    std::string content =
    trim(
        line.substr(6)
    );

    auto colon =
        content.find(':');

    if(colon == std::string::npos)
    {
        return false;
    }

    std::string output =
    trim(
        content.substr(
            0,
            colon
        )
    );

    std::string rest =
    trim(
        content.substr(
            colon + 1
        )
    );

    auto parts =
    split(
        rest,
        ' '
    );

    if(parts.size() < 2)
    {
        return false;
    }

    Rule* rule =
        manifest.find_rule(
            parts[0]
        );

    if(rule == nullptr)
    {
        return false;
    }

    Edge* edge =
        manifest.graph()
                .create_edge(
                    rule
                );

    Node* output_node =
        manifest.graph()
                .get_or_create_node(
                    output
                );

    manifest.graph()
        .add_output(
            edge,
            output_node
        );

    for(size_t i = 1;
        i < parts.size();
        i++)
    {

        Node* input_node =
            manifest.graph()
                    .get_or_create_node(
                        parts[i]
                    );


        manifest.graph()
            .add_input(
                edge,
                input_node
            );

    }

    return true;

}


}