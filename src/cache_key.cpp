#include "forge/cache_key.hpp"


namespace forge
{


namespace
{


constexpr std::string_view cache_version =
    "ForgeBuildCacheV1";


} // namespace


CacheKeyBuilder::CacheKeyBuilder()
{
    add_field(
        "version",
        cache_version
    );
}


void CacheKeyBuilder::add_command(
    std::string_view command
)
{
    add_field(
        "command",
        command
    );
}

bool CacheKeyBuilder::add_dependency(
    const std::string& path
)
{
    add_field(
        "dependency_path",
        path
    );


    hasher_.update(
        "dependency_content"
    );


    hasher_.update(
        std::string_view(
            "\0",
            1
        )
    );


    if (!hasher_.update_file(
            path
        ))
    {
        return false;
    }


    hasher_.update(
        std::string_view(
            "\0",
            1
        )
    );


    return true;
}


std::uint64_t CacheKeyBuilder::value() const
{
    return hasher_.value();
}


void CacheKeyBuilder::add_field(
    std::string_view name,
    std::string_view value
)
{
    hasher_.update(
        name
    );


    hasher_.update(
        std::string_view(
            "\0",
            1
        )
    );


    hasher_.update(
        value
    );


    hasher_.update(
        std::string_view(
            "\0",
            1
        )
    );
}


} // namespace forge