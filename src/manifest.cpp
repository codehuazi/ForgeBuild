#include "forge/manifest.hpp"

#include "forge/rule.hpp"

#include <stdexcept>
#include <utility>

namespace forge {

Manifest::Manifest() = default;

Manifest::~Manifest() = default;

Rule* Manifest::add_rule(
    std::string name,
    std::string command
)
{
    if (rules_.contains(name)) {
        throw std::runtime_error(
            "rule already exists: " + name
        );
    }

    auto rule = std::make_unique<Rule>(
        std::move(name),
        std::move(command)
    );

    Rule* rule_ptr = rule.get();

    rules_.emplace(
        rule_ptr->name(),
        std::move(rule)
    );

    return rule_ptr;
}

Rule* Manifest::find_rule(
    const std::string& name
) const
{
    auto it = rules_.find(name);

    if (it == rules_.end()) {
        return nullptr;
    }

    return it->second.get();
}

BuildGraph& Manifest::graph()
{
    return graph_;
}

const BuildGraph& Manifest::graph() const
{
    return graph_;
}

} // namespace forge