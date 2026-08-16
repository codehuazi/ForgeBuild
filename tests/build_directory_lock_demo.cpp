#include "forge/build_directory_lock.hpp"

#include <cerrno>
#include <filesystem>
#include <iostream>
#include <string>


int main()
{
    namespace fs =
        std::filesystem;


    const std::string root =
        ".forge_build_directory_lock_test";


    const std::string lock_path =
        root + "/.forge_lock";


    fs::remove_all(
        root
    );


    fs::create_directory(
        root
    );


    /*
     * 第一个对象成功获得排他锁；
     * 第二个独立 open 同一文件时必须失败。
     */
    {
        forge::BuildDirectoryLock first(
            lock_path
        );


        if(!first.try_acquire()
            || !first.acquired())
        {
            std::cerr
                << "first lock acquisition failed\n";

            fs::remove_all(
                root
            );

            return 1;
        }


        forge::BuildDirectoryLock second(
            lock_path
        );


        if(second.try_acquire())
        {
            std::cerr
                << "second lock unexpectedly succeeded\n";

            fs::remove_all(
                root
            );

            return 1;
        }


        if(second.error_number()
            != EWOULDBLOCK)
        {
            std::cerr
                << "unexpected lock failure error: "
                << second.error_number()
                << '\n';

            fs::remove_all(
                root
            );

            return 1;
        }
    }


    /*
     * first 已经离开作用域并析构。
     * RAII 必须释放锁，所以第三个对象应该成功。
     */
    {
        forge::BuildDirectoryLock third(
            lock_path
        );


        if(!third.try_acquire()
            || !third.acquired())
        {
            std::cerr
                << "lock was not released by RAII\n";

            fs::remove_all(
                root
            );

            return 1;
        }
    }


    /*
     * 锁文件仍然存在完全正常。
     * 是否被锁取决于内核 flock 状态，
     * 不是文件是否存在。
     */
    if(!fs::exists(
            lock_path
        ))
    {
        std::cerr
            << "lock file unexpectedly disappeared\n";

        fs::remove_all(
            root
        );

        return 1;
    }


    fs::remove_all(
        root
    );


    std::cout
        << "build directory lock checks passed\n";


    return 0;
}