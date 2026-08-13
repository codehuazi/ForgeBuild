#include "forge/compiler_identity.hpp"

#include "forge/hash.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include <unistd.h>


namespace forge
{


namespace
{


bool extract_compiler(
    const std::string& command,
    std::string& compiler
)
{
    std::istringstream stream(
        command
    );


    return static_cast<bool>(
        stream >> compiler
    );
}


bool resolve_executable(
    const std::string& executable,
    std::filesystem::path& resolved
)
{
    namespace fs = std::filesystem;


    const fs::path executable_path(
        executable
    );


    //
    // 显式路径：
    //
    // /usr/bin/g++
    // ./my_compiler
    //
    if(executable_path.has_parent_path())
    {
        if(::access(
                executable.c_str(),
                X_OK
            ) != 0)
        {
            return false;
        }


        std::error_code error;

        resolved =
            fs::canonical(
                executable_path,
                error
            );


        return !error;
    }


    //
    // 普通命令名：
    //
    // g++
    // clang++
    //
    // 从 PATH 中查找。
    //
    const char* path_environment =
        std::getenv(
            "PATH"
        );


    if(path_environment == nullptr)
    {
        return false;
    }


    std::stringstream path_stream(
        path_environment
    );


    std::string directory;


    while(std::getline(
        path_stream,
        directory,
        ':'
    ))
    {
        if(directory.empty())
        {
            directory = ".";
        }


        const fs::path candidate =
            fs::path(directory)
            / executable;


        const std::string candidate_string =
            candidate.string();


        if(::access(
                candidate_string.c_str(),
                X_OK
            ) != 0)
        {
            continue;
        }


        std::error_code error;

        const fs::path canonical =
            fs::canonical(
                candidate,
                error
            );


        if(error)
        {
            continue;
        }


        resolved = canonical;

        return true;
    }


    return false;
}


} // namespace


bool CompilerIdentityCache::get(
    const std::string& command,
    std::string& identity
)
{
    std::string compiler;


    if(!extract_compiler(
            command,
            compiler
        ))
    {
        return false;
    }


    //
    // 同一个 CompilerIdentityCache 会被多个
    // Scheduler Worker 共享。
    //
    // 这里连首次 identity 计算也放在锁内，
    // 保证同一 compiler 不会被多个 Worker
    // 同时重复计算。
    //
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    const auto iterator =
        identities_.find(
            compiler
        );


    if(iterator != identities_.end())
    {
        identity =
            iterator->second;

        return true;
    }


    std::string computed_identity;


    if(!compute_identity(
            compiler,
            computed_identity
        ))
    {
        return false;
    }


    identities_.emplace(
        compiler,
        computed_identity
    );


    identity =
        std::move(
            computed_identity
        );


    return true;
}


bool CompilerIdentityCache::compute_identity(
    const std::string& compiler,
    std::string& identity
) const
{
    std::filesystem::path resolved;


    if(!resolve_executable(
            compiler,
            resolved
        ))
    {
        return false;
    }


    Hash64 hasher;


    if(!hasher.update_file(
            resolved.string()
        ))
    {
        return false;
    }


    identity =
        resolved.string()
        + "|"
        + std::to_string(
            hasher.value()
        );


    return true;
}


} // namespace forge