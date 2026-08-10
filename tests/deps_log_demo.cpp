#include "forge/deps_log.hpp"

#include <filesystem>
#include <iostream>


int main()
{
    const std::string log_path =
        ".forge_deps_demo";


    /*
     * 第一阶段：
     * 创建并保存依赖记录。
     */

    forge::DepsLog saved_deps;


    saved_deps.record(
        "main.o",
        {
            "main.cpp",
            "config.h",
            "math.h"
        }
    );


    saved_deps.record(
        "app",
        {
            "main.o"
        }
    );


    if (!saved_deps.save(log_path))
    {
        std::cerr
            << "failed to save deps log\n";

        return 1;
    }


    /*
     * 第二阶段：
     * 创建一个全新的 DepsLog，
     * 模拟程序重新启动。
     */

    forge::DepsLog loaded_deps;


    if (!loaded_deps.load(log_path))
    {
        std::cerr
            << "failed to load deps log\n";

        return 1;
    }


    /*
     * 第三阶段：
     * 检查从磁盘恢复的数据。
     */

    for (const std::string& output :
         {"main.o", "app"})
    {
        if (!loaded_deps.contains(output))
        {
            std::cerr
                << "missing output: "
                << output
                << '\n';

            return 1;
        }


        std::cout
            << output
            << " dependencies:\n";


        for (const std::string& input :
             loaded_deps.inputs(output))
        {
            std::cout
                << "  "
                << input
                << '\n';
        }
    }


    std::filesystem::remove(
        log_path
    );


    return 0;
}