#include "forge/build_log.hpp"

#include "forge/file_system.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace forge
{

namespace
{


constexpr const char* build_log_header =
    "FORGEBUILD_BUILD_LOG_V1";


}

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


void BuildLog::clear()
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    entries_.clear();
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


    output
        << build_log_header
        << '\n';


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

LogLoadResult BuildLog::load(
    const std::string& file_path
)
{
    std::error_code exists_error;


    const bool file_exists =
        std::filesystem::exists(
            file_path,
            exists_error
        );


    if(exists_error)
    {
        return LogLoadResult::IoError;
    }


    if(!file_exists)
    {
        return LogLoadResult::Missing;
    }


    std::ifstream input(
        file_path,
        std::ios::binary
    );


    if(!input)
    {
        return LogLoadResult::IoError;
    }


    std::string header;


    if(!std::getline(
            input,
            header
        ))
    {
        return LogLoadResult::Corrupted;
    }


    if(header != build_log_header)
    {
        return LogLoadResult::
            UnsupportedVersion;
    }


    std::unordered_map<
        std::string,
        std::uint64_t
    > loaded_entries;


    std::string line;


    while(std::getline(
        input,
        line
    ))
    {
        if(line.empty())
        {
            return LogLoadResult::Corrupted;
        }


        const std::size_t separator =
            line.find('\t');


        if(separator
            == std::string::npos)
        {
            return LogLoadResult::Corrupted;
        }


        const std::string output_path =
            line.substr(
                0,
                separator
            );


        const std::string hash_text =
            line.substr(
                separator + 1
            );


        if(output_path.empty()
            || hash_text.empty())
        {
            return LogLoadResult::Corrupted;
        }


        std::istringstream hash_stream(
            hash_text
        );


        std::uint64_t hash = 0;


        hash_stream >> hash;


        if(!hash_stream)
        {
            return LogLoadResult::Corrupted;
        }


        /*
         * 不能只判断数字前半段是否能解析。
         *
         * 例如：
         *
         *   123abc
         *
         * operator>> 仍然可能成功读出 123。
         *
         * 所以这里继续吃掉尾部空白，并要求流必须
         * 真正到达结尾。
         */
        hash_stream >> std::ws;


        if(!hash_stream.eof())
        {
            return LogLoadResult::Corrupted;
        }


        if(!loaded_entries.emplace(
                output_path,
                hash
            ).second)
        {
            /*
             * 同一个 Output 在 BuildLog 中出现两次，
             * 说明持久化文件自身不一致。
             */
            return LogLoadResult::Corrupted;
        }
    }


    if(!input.eof())
    {
        return LogLoadResult::IoError;
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


    return LogLoadResult::Ok;
}

}