#pragma once

#include "forge/hash.hpp"

#include <string>
#include <cstdint>
#include <string_view>


namespace forge
{


class CacheKeyBuilder
{

public:

    CacheKeyBuilder();

    void add_compiler_identity(
        std::string_view compiler_identity
    );

    void add_command(
        std::string_view command
    );

    bool add_dependency(
        const std::string& path
    );


    std::uint64_t value() const;


private:

    void add_field(
        std::string_view name,
        std::string_view value
    );


    Hash64 hasher_;

};


} // namespace forge