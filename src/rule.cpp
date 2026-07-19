#include "forge/rule.hpp"

#include <utility>

namespace forge {

Rule::Rule(std::string name, std::string command)
    : name_(std::move(name)),
      command_(std::move(command))
{
}

const std::string& Rule::name() const
{
    return name_;
}

const std::string& Rule::command() const
{
    return command_;
}

} // namespace forge