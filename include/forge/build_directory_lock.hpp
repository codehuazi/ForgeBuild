#pragma once

#include <string>


namespace forge
{


class BuildDirectoryLock
{
public:

    explicit BuildDirectoryLock(
        std::string path
    );


    ~BuildDirectoryLock();


    BuildDirectoryLock(
        const BuildDirectoryLock&
    ) = delete;


    BuildDirectoryLock& operator=(
        const BuildDirectoryLock&
    ) = delete;


    BuildDirectoryLock(
        BuildDirectoryLock&&
    ) = delete;


    BuildDirectoryLock& operator=(
        BuildDirectoryLock&&
    ) = delete;


    bool try_acquire();


    bool acquired() const noexcept;


    int error_number() const noexcept;


private:

    std::string path_;


    int fd_{
        -1
    };


    bool acquired_{
        false
    };


    int error_number_{
        0
    };
};


}