#pragma once

#include <string>


namespace forge
{


class FileSystem
{

public:

    static bool exists(
        const std::string& path
    );


    static long long timestamp(
        const std::string& path
    );


    //
    // 将完整内容先写入与目标文件同目录的临时文件，
    // 成功关闭后再 rename 到最终路径。
    //
    // 这样进程在写入过程中异常退出时，
    // 不会先截断原来的最终文件。
    //
    static bool atomic_write_file(
        const std::string& path,
        const std::string& content
    );

};


}