#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>


namespace forge
{


class DepsLog
{

public:

    void record(
        const std::string& output,
        const std::vector<std::string>& inputs
    );


    bool save(
        const std::string& file_path
    ) const;


    bool load(
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