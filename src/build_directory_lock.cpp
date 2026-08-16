#include "forge/build_directory_lock.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <utility>


namespace forge
{


BuildDirectoryLock::BuildDirectoryLock(
    std::string path
)
    :
    path_(
        std::move(
            path
        )
    )
{
}


BuildDirectoryLock::~BuildDirectoryLock()
{
    if(fd_ < 0)
    {
        return;
    }


    if(acquired_)
    {
        /*
         * close(fd_) 本身也会释放 flock，
         * 这里显式 LOCK_UN 让资源生命周期更清楚。
         */
        static_cast<void>(
            ::flock(
                fd_,
                LOCK_UN
            )
        );
    }


    static_cast<void>(
        ::close(
            fd_
        )
    );
}


bool BuildDirectoryLock::try_acquire()
{
    if(acquired_)
    {
        return true;
    }


    fd_ =
        ::open(
            path_.c_str(),
            O_CREAT
                | O_RDWR
                | O_CLOEXEC,
            0666
        );


    if(fd_ < 0)
    {
        error_number_ =
            errno;


        return false;
    }


    int lock_result =
        -1;


    do
    {
        lock_result =
            ::flock(
                fd_,
                LOCK_EX
                    | LOCK_NB
            );
    }
    while(lock_result == -1
        && errno == EINTR);


    if(lock_result == -1)
    {
        const int lock_error =
            errno;


        static_cast<void>(
            ::close(
                fd_
            )
        );


        fd_ =
            -1;


        error_number_ =
            lock_error;


        return false;
    }


    acquired_ =
        true;


    error_number_ =
        0;


    return true;
}


bool BuildDirectoryLock::acquired() const noexcept
{
    return acquired_;
}


int BuildDirectoryLock::error_number() const noexcept
{
    return error_number_;
}


}