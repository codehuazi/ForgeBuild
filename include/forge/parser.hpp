#pragma once

#include <cstddef>
#include <string>


namespace forge
{


class Manifest;
class Rule;


class Parser
{
public:

    bool parse_file(
        const std::string& path,
        Manifest& manifest
    );


    const std::string& error() const;


private:

    bool parse_line(
        const std::string& line,
        Manifest& manifest
    );


    bool parse_build(
        const std::string& line,
        Manifest& manifest
    );


    bool fail(
        const std::string& message
    );


private:

    enum class State
    {
        None,
        Rule,
        Build
    };


    State state_ =
        State::None;


    Rule* current_rule_ =
        nullptr;


    std::string current_path_;

    std::size_t current_line_ =
        0;


    std::string error_;
};


}