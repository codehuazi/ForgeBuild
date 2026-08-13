#pragma once

#include <mutex>
#include <string>
#include <unordered_map>


namespace forge
{


class CompilerIdentityCache
{

public:

    bool get(
        const std::string& command,
        std::string& identity
    );


private:

    bool compute_identity(
        const std::string& compiler,
        std::string& identity
    ) const;


    std::mutex mutex_;

    std::unordered_map<
        std::string,
        std::string
    > identities_;

};


} // namespace forge