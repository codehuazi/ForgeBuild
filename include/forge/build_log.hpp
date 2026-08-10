#pragma once

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

    bool load(
        const std::string& file_path
    );

    bool save(
        const std::string& file_path
    ) const;


private:

    //
    // const 成员函数也需要保护 entries_，
    // 因此 mutex_ 声明为 mutable。
    //
    mutable std::mutex mutex_;


    std::unordered_map<
        std::string,
        std::uint64_t
    > entries_;
};

}