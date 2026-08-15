#include "forge/deps_log.hpp"

#include "forge/file_system.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>


namespace forge
{

namespace
{


constexpr const char* deps_log_header =
    "FORGEBUILD_DEPS_LOG_V1";


}

void DepsLog::record(
    const std::string& output,
    const std::vector<std::string>& inputs
)
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    entries_[output] =
        inputs;
}

void DepsLog::clear()
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    entries_.clear();
}

bool DepsLog::save(
    const std::string& file_path
) const
{
    std::unordered_map<
        std::string,
        std::vector<std::string>
    > snapshot;


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        snapshot =
            entries_;
    }


    std::ostringstream output_file;


    output_file
        << deps_log_header
        << '\n';


    for(const auto& [output, inputs] :
        snapshot)
    {
        output_file
            << output
            << '\n';


        output_file
            << inputs.size()
            << '\n';


        for(const std::string& input :
            inputs)
        {
            output_file
                << input
                << '\n';
        }
    }


    if(!output_file)
    {
        return false;
    }


    return FileSystem::atomic_write_file(
        file_path,
        output_file.str()
    );
}


LogLoadResult DepsLog::load(
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


    std::ifstream input_file(
        file_path,
        std::ios::binary
    );


    if(!input_file)
    {
        return LogLoadResult::IoError;
    }


    std::string header;


    if(!std::getline(
            input_file,
            header
        ))
    {
        return LogLoadResult::Corrupted;
    }


    if(header != deps_log_header)
    {
        return LogLoadResult::
            UnsupportedVersion;
    }


    std::unordered_map<
        std::string,
        std::vector<std::string>
    > loaded_entries;


    std::string output_path;


    while(std::getline(
        input_file,
        output_path
    ))
    {
        if(output_path.empty())
        {
            return LogLoadResult::Corrupted;
        }


        std::string count_text;


        if(!std::getline(
                input_file,
                count_text
            ))
        {
            return LogLoadResult::Corrupted;
        }


        std::istringstream count_stream(
            count_text
        );


        std::size_t input_count = 0;


        count_stream >> input_count;


        if(!count_stream)
        {
            return LogLoadResult::Corrupted;
        }


        count_stream >> std::ws;


        if(!count_stream.eof())
        {
            return LogLoadResult::Corrupted;
        }


        std::vector<std::string> inputs;


        for(std::size_t i = 0;
            i < input_count;
            ++i)
        {
            std::string input_path;


            if(!std::getline(
                    input_file,
                    input_path
                ))
            {
                return LogLoadResult::Corrupted;
            }


            if(input_path.empty())
            {
                return LogLoadResult::Corrupted;
            }


            inputs.push_back(
                std::move(
                    input_path
                )
            );
        }


        if(!loaded_entries.emplace(
                output_path,
                std::move(
                    inputs
                )
            ).second)
        {
            return LogLoadResult::Corrupted;
        }
    }


    if(!input_file.eof())
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


bool DepsLog::contains(
    const std::string& output
) const
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    return entries_.find(output)
        != entries_.end();
}


std::vector<std::string> DepsLog::inputs(
    const std::string& output
) const
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    return entries_.at(output);
}


}