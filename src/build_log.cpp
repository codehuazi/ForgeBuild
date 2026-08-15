#include "forge/build_log.hpp"

#include "forge/file_system.hpp"

#include <fstream>
#include <sstream>
#include <utility>

namespace forge
{

void BuildLog::record(
    const std::string& output_path,
    std::uint64_t command_hash
)
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    entries_[output_path] =
        command_hash;
}


bool BuildLog::contains(
    const std::string& output_path
) const
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    return entries_.find(
        output_path
    ) != entries_.end();
}


std::uint64_t BuildLog::command_hash(
    const std::string& output_path
) const
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    const auto iterator =
        entries_.find(
            output_path
        );


    if(iterator == entries_.end())
    {
        return 0;
    }


    return iterator->second;
}


bool BuildLog::command_matches(
    const std::string& output_path,
    std::uint64_t current_hash
) const
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    const auto iterator =
        entries_.find(
            output_path
        );


    if(iterator == entries_.end())
    {
        return false;
    }


    return iterator->second
        == current_hash;
}

bool BuildLog::save(
    const std::string& file_path
) const
{
    std::unordered_map<
        std::string,
        std::uint64_t
    > snapshot;


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        snapshot =
            entries_;
    }


    std::ostringstream output;


    for(const auto& entry :
        snapshot)
    {
        output
            << entry.first
            << '\t'
            << entry.second
            << '\n';
    }


    if(!output)
    {
        return false;
    }


    return FileSystem::atomic_write_file(
        file_path,
        output.str()
    );
}

bool BuildLog::load(
    const std::string& file_path
)
{
    std::ifstream input(file_path);

    if (!input)
    {
        return true;
    }


    std::unordered_map<
        std::string,
        std::uint64_t
    > loaded_entries;


    std::string line;

    while (std::getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }


        const std::size_t separator =
            line.find('\t');

        if (separator == std::string::npos)
        {
            return false;
        }


        const std::string output_path =
            line.substr(0, separator);

        const std::string hash_text =
            line.substr(separator + 1);


        if (output_path.empty()
            || hash_text.empty())
        {
            return false;
        }


        std::istringstream hash_stream(
            hash_text
        );

        std::uint64_t hash = 0;

        hash_stream >> hash;


        if (!hash_stream)
        {
            return false;
        }


        loaded_entries[output_path] =
            hash;
    }


    if (!input.eof())
    {
        return false;
    }


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        entries_ =
            std::move(
                loaded_entries
            );
    }


    return true;
}

}