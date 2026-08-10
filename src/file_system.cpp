#include "forge/file_system.hpp"

#include <filesystem>


namespace forge
{


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


    auto time =
        std::filesystem::last_write_time(
            path
        );


    return time.time_since_epoch()
        .count();

}

}