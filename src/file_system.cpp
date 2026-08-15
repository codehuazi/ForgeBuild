#include "forge/file_system.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>


namespace forge
{


namespace
{


std::atomic<std::uint64_t>
    next_temp_id{0};


std::string temporary_path(
    const std::string& final_path
)
{
    const std::uint64_t temp_id =
        next_temp_id.fetch_add(
            1,
            std::memory_order_relaxed
        );


    return final_path
        + ".tmp."
        + std::to_string(
            static_cast<long long>(
                ::getpid()
            )
        )
        + "."
        + std::to_string(
            temp_id
        );
}


void remove_if_exists(
    const std::string& path
)
{
    std::error_code error;


    std::filesystem::remove(
        path,
        error
    );
}


} // namespace


bool FileSystem::exists(
    const std::string& path
)
{
    return std::filesystem::exists(
        path
    );
}


long long FileSystem::timestamp(
    const std::string& path
)
{
    if(!exists(path))
    {
        return 0;
    }


    const auto time =
        std::filesystem::last_write_time(
            path
        );


    return time.time_since_epoch()
        .count();
}


bool FileSystem::atomic_write_file(
    const std::string& path,
    const std::string& content
)
{
    const std::string temp_path =
        temporary_path(
            path
        );


    remove_if_exists(
        temp_path
    );


    {
        std::ofstream output(
            temp_path,
            std::ios::binary
            | std::ios::trunc
        );


        if(!output)
        {
            return false;
        }


        output.write(
            content.data(),
            static_cast<std::streamsize>(
                content.size()
            )
        );


        output.close();


        if(!output)
        {
            remove_if_exists(
                temp_path
            );

            return false;
        }
    }


    std::error_code error;


    std::filesystem::rename(
        temp_path,
        path,
        error
    );


    if(error)
    {
        remove_if_exists(
            temp_path
        );

        return false;
    }


    return true;
}


}