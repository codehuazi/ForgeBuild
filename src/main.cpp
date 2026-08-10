#include "forge/build_log.hpp"
#include "forge/build_plan.hpp"
#include "forge/builder.hpp"
#include "forge/deps_log.hpp"
#include "forge/executor.hpp"
#include "forge/manifest.hpp"
#include "forge/parser.hpp"
#include "forge/scheduler.hpp"
#include "forge/local_cache.hpp"

#include <iostream>
#include <string>


namespace
{


struct CommandLineOptions
{
    int jobs = 1;

    std::string manifest_path;
};


void print_usage()
{
    std::cerr
        << "usage: forge [-j N | -jN] <manifest-file>\n";
}


bool parse_positive_integer(
    const std::string& text,
    int& value
)
{
    if (text.empty())
    {
        return false;
    }


    std::size_t position = 0;


    try
    {
        const int parsed =
            std::stoi(
                text,
                &position
            );


        if (position != text.size()
            || parsed <= 0)
        {
            return false;
        }


        value = parsed;

        return true;
    }
    catch (...)
    {
        return false;
    }
}


bool parse_command_line(
    int argc,
    char* argv[],
    CommandLineOptions& options
)
{
    for (int i = 1;
         i < argc;
         ++i)
    {
        const std::string argument =
            argv[i];


        if (argument == "-j")
        {
            if (i + 1 >= argc)
            {
                std::cerr
                    << "missing value after -j\n";

                return false;
            }


            ++i;


            if (!parse_positive_integer(
                    argv[i],
                    options.jobs
                ))
            {
                std::cerr
                    << "invalid job count: "
                    << argv[i]
                    << '\n';

                return false;
            }


            continue;
        }


        if (argument.starts_with("-j")
            && argument.size() > 2)
        {
            if (!parse_positive_integer(
                    argument.substr(2),
                    options.jobs
                ))
            {
                std::cerr
                    << "invalid job count: "
                    << argument.substr(2)
                    << '\n';

                return false;
            }


            continue;
        }


        if (!argument.empty()
            && argument[0] == '-')
        {
            std::cerr
                << "unknown option: "
                << argument
                << '\n';

            return false;
        }


        if (!options.manifest_path.empty())
        {
            std::cerr
                << "multiple manifest files provided\n";

            return false;
        }


        options.manifest_path =
            argument;
    }


    if (options.manifest_path.empty())
    {
        std::cerr
            << "manifest file is required\n";

        return false;
    }


    return true;
}


}


int main(
    int argc,
    char* argv[]
)
{
    /*
     * 用法：
     *
     *   forge build.forge
     */

    CommandLineOptions options;


    if (!parse_command_line(
            argc,
            argv,
            options
        ))
    {
        print_usage();

        return 1;
    }


    const std::string& manifest_path =
        options.manifest_path;


    const int jobs =
        options.jobs;


    const std::string build_log_path =
        ".forge_log";

    const std::string deps_log_path =
        ".forge_deps";



    std::cout
        << "jobs: "
        << jobs
        << '\n';

    std::cout
        << "manifest: "
        << manifest_path
        << '\n';

    /*
     * 第一阶段：
     * 解析 Forgefile，生成 Manifest 和 BuildGraph。
     */

    forge::Manifest manifest;

    forge::Parser parser;


    if (!parser.parse_file(
            manifest_path,
            manifest
        ))
    {
        std::cerr
            << "failed to parse manifest: "
            << manifest_path
            << '\n';

        return 1;
    }


    /*
     * 第二阶段：
     * 加载上一次构建留下的状态。
     *
     * 文件第一次不存在是正常情况，
     * 所以 load() 失败时不直接退出。
     */

    forge::BuildLog build_log;

    forge::DepsLog deps_log;


    build_log.load(
        build_log_path
    );


    deps_log.load(
        deps_log_path
    );


    /*
     * 第三阶段：
     * 根据文件状态、命令哈希和动态依赖，
     * 生成本次最小构建计划。
     */

    forge::Builder builder(
        manifest.graph(),
        build_log,
        deps_log
    );


    forge::BuildPlan plan =
        builder.build();


    std::cout
        << "planned edge count: "
        << plan.edges().size()
        << '\n';


    if (plan.edges().empty())
    {
        std::cout
            << "nothing to build\n";

        return 0;
    }


    /*
    * 第四阶段：
    * 通过 Scheduler 执行构建计划。
    *
    * jobs == 1 时是单 Worker；
    * jobs > 1 时可以并行执行互不依赖的 Edge。
    */

    forge::LocalCache local_cache(
        ".forge_cache/objects"
    );


    forge::Executor executor(
        build_log,
        deps_log,
        local_cache
    );


    forge::Scheduler scheduler(
        jobs
    );


    scheduler.set_executor(
        &executor
    );


    if (!scheduler.run(
            plan.edges()
        ))
    {
        std::cerr
            << "build failed\n";

        return 1;
    }


    /*
     * 第五阶段：
     * 只有构建成功后才保存状态。
     *
     * 如果中途失败，不应覆盖上一份可靠日志。
     */

    if (!build_log.save(
            build_log_path
        ))
    {
        std::cerr
            << "failed to save build log\n";

        return 1;
    }


    if (!deps_log.save(
            deps_log_path
        ))
    {
        std::cerr
            << "failed to save deps log\n";

        return 1;
    }


    std::cout
        << "build succeeded\n";


    return 0;
}