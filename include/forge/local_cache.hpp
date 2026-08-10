#pragma once

#include <cstdint>
#include <string>


namespace forge
{


class LocalCache
{

public:

    explicit LocalCache(
        std::string root
    );


    bool contains(
        std::uint64_t key
    ) const;


    bool store(
        std::uint64_t key,
        const std::string& output
    );


    bool restore(
        std::uint64_t key,
        const std::string& output
    ) const;


private:

    std::string object_path(
        std::uint64_t key
    ) const;


    std::string metadata_path(
        std::uint64_t key
    ) const;


    std::string root_;

};


} // namespace forge