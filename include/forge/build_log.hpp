#pragma once

#include "forge/log_load_result.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>


namespace forge
{


class BuildLog
{
public:

    void record(
        const std::string& output_path,
        std::uint64_t command_hash
    );


    bool contains(
        const std::string& output_path
    ) const;


    std::uint64_t command_hash(
        const std::string& output_path
    ) const;


    bool command_matches(
        const std::string& output_path,
        std::uint64_t current_hash
    ) const;


    void clear();


    LogLoadResult load(
        const std::string& file_path
    );


    bool save(
        const std::string& file_path
    ) const;


private:

    mutable std::mutex mutex_;


    std::unordered_map<
        std::string,
        std::uint64_t
    > entries_;
};


}