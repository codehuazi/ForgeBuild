#pragma once

#include <string>


namespace forge {

class Manifest;

class Rule;

class Parser
{
public:

    bool parse_file(
        const std::string& path,
        Manifest& manifest
    );


private:

    bool parse_line(
        const std::string& line,
        Manifest& manifest
    );

    bool parse_build(
        const std::string& line,
        Manifest& manifest
    );

private:

    enum class State
    {
        None,
        Rule,
        Build
    };


    State state_ = State::None;

    Rule* current_rule_ = nullptr;

};


}