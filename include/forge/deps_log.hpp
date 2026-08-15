#pragma once

#include "forge/log_load_result.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>


namespace forge
{


class DepsLog
{

public:

    void record(
        const std::string& output,
        const std::vector<std::string>& inputs
    );


    void clear();


    bool save(
        const std::string& file_path
    ) const;


    LogLoadResult load(
        const std::string& file_path
    );


    bool contains(
        const std::string& output
    ) const;


    std::vector<std::string> inputs(
        const std::string& output
    ) const;


private:

    mutable std::mutex mutex_;


    std::unordered_map<
        std::string,
        std::vector<std::string>
    > entries_;
};


}