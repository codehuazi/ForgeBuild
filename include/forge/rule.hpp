#pragma once

#include <string>

namespace forge {

class Rule
{
public:
    Rule(std::string name, std::string command);

    const std::string& name() const;

    const std::string& command() const;

private:
    std::string name_;

    std::string command_;
};

} // namespace forge