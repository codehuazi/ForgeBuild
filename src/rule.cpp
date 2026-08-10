#include "forge/rule.hpp"

#include <utility>


namespace forge
{


Rule::Rule(
    std::string name,
    std::string command
)
    :
    name_(std::move(name)),
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


void Rule::set_command(
    std::string command
)
{
    command_ =
        std::move(command);
}


const std::string& Rule::depfile() const
{
    return depfile_;
}


void Rule::set_depfile(
    std::string depfile
)
{
    depfile_ =
        std::move(depfile);
}


}