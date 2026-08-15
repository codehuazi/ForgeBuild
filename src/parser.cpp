#include "forge/parser.hpp"

#include "forge/build_graph.hpp"
#include "forge/edge.hpp"
#include "forge/manifest.hpp"
#include "forge/node.hpp"
#include "forge/rule.hpp"
#include "forge/string_utils.hpp"

#include <exception>
#include <fstream>


namespace forge
{


bool Parser::parse_file(
    const std::string& path,
    Manifest& manifest
)
{
    /*
     * Parser 对象允许重复使用，因此每次解析新文件时
     * 都必须重置内部状态。
     */
    state_ =
        State::None;


    current_rule_ =
        nullptr;


    current_path_ =
        path;


    current_line_ =
        0;


    error_.clear();


    std::ifstream file(
        path
    );


    if(!file.is_open())
    {
        return fail(
            "cannot open file"
        );
    }


    std::string line;


    while(std::getline(
        file,
        line
    ))
    {
        ++current_line_;


        try
        {
            if(!parse_line(
                    line,
                    manifest
                ))
            {
                return false;
            }
        }
        catch(const std::exception& exception)
        {
            return fail(
                exception.what()
            );
        }
    }


    if(file.bad())
    {
        return fail(
            "I/O error while reading manifest"
        );
    }


    return true;
}


const std::string& Parser::error() const
{
    return error_;
}


bool Parser::fail(
    const std::string& message
)
{
    error_ =
        current_path_;


    if(current_line_ != 0)
    {
        error_
            += ":"
            + std::to_string(
                current_line_
            );
    }


    error_
        += ": "
        + message;


    return false;
}


bool Parser::parse_line(
    const std::string& raw,
    Manifest& manifest
)
{
    const std::string line =
        trim(
            raw
        );


    /*
     * 空行和整行注释直接跳过。
     */
    if(line.empty()
        || starts_with(
            line,
            "#"
        ))
    {
        return true;
    }


    if(starts_with(
        line,
        "rule "
    ))
    {
        const std::string name =
            trim(
                line.substr(5)
            );


        if(name.empty())
        {
            return fail(
                "rule name must not be empty"
            );
        }


        current_rule_ =
            manifest.add_rule(
                name,
                ""
            );


        state_ =
            State::Rule;


        return true;
    }


    if(starts_with(
        line,
        "command ="
    ))
    {
        if(state_
                != State::Rule
            || current_rule_
                == nullptr)
        {
            return fail(
                "command must follow a rule"
            );
        }


        const std::string command =
            trim(
                line.substr(9)
            );


        if(command.empty())
        {
            return fail(
                "command must not be empty"
            );
        }


        current_rule_->set_command(
            command
        );


        return true;
    }


    if(starts_with(
        line,
        "depfile ="
    ))
    {
        if(state_
                != State::Rule
            || current_rule_
                == nullptr)
        {
            return fail(
                "depfile must follow a rule"
            );
        }


        const std::string depfile =
            trim(
                line.substr(9)
            );


        if(depfile.empty())
        {
            return fail(
                "depfile must not be empty"
            );
        }


        current_rule_->set_depfile(
            depfile
        );


        return true;
    }


    if(starts_with(
        line,
        "build "
    ))
    {
        if(!parse_build(
                line,
                manifest
            ))
        {
            return false;
        }


        /*
         * build 语句结束后不能再通过 command =
         * 修改前一个 Rule。
         */
        state_ =
            State::Build;


        current_rule_ =
            nullptr;


        return true;
    }


    return fail(
        "unknown statement: "
        + line
    );
}


bool Parser::parse_build(
    const std::string& line,
    Manifest& manifest
)
{
    const std::string content =
        trim(
            line.substr(6)
        );


    const std::size_t colon =
        content.find(':');


    if(colon
        == std::string::npos)
    {
        return fail(
            "build statement is missing ':'"
        );
    }


    const std::string output_text =
        trim(
            content.substr(
                0,
                colon
            )
        );


    const std::vector<std::string>
        output_paths =
            split(
                output_text
            );


    if(output_paths.empty())
    {
        return fail(
            "build statement requires at least one output"
        );
    }


    const std::string rest =
        trim(
            content.substr(
                colon + 1
            )
        );


    const std::vector<std::string> parts =
        split(
            rest
        );


    if(parts.empty())
    {
        return fail(
            "build statement requires a rule"
        );
    }


    if(parts.size() < 2)
    {
        return fail(
            "build statement requires at least one input"
        );
    }


    Rule* rule =
        manifest.find_rule(
            parts[0]
        );


    if(rule == nullptr)
    {
        return fail(
            "unknown rule: "
            + parts[0]
        );
    }


    if(rule->command().empty())
    {
        return fail(
            "rule has no command: "
            + rule->name()
        );
    }


    Edge* edge =
        manifest.graph()
            .create_edge(
                rule
            );


    /*
     * 一个 build 语句现在可以声明多个 Output。
     */
    for(const std::string& output_path :
        output_paths)
    {
        Node* output_node =
            manifest.graph()
                .get_or_create_node(
                    output_path
                );


        manifest.graph()
            .add_output(
                edge,
                output_node
            );
    }


    for(std::size_t i = 1;
        i < parts.size();
        ++i)
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