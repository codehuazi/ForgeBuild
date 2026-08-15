#include "forge/build_log.hpp"
#include "forge/build_plan.hpp"
#include "forge/build_graph_validator.hpp"
#include "forge/builder.hpp"
#include "forge/deps_log.hpp"
#include "forge/executor.hpp"
#include "forge/manifest.hpp"
#include "forge/parser.hpp"
#include "forge/scheduler.hpp"
#include "forge/local_cache.hpp"
#include "forge/edge.hpp"
#include "forge/file_system.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <exception>


namespace
{


struct CommandLineOptions
{
    int jobs = 1;

    bool explain = false;

    std::string manifest_path;
};


void print_usage()
{
    std::cerr
        << "usage: forge [--explain] [-j N | -jN] <manifest-file>\n";
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

        if (argument
            == "--explain")
        {
            options.explain = true;

            continue;
        }

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

    const std::string state_marker_path =
        ".forge_in_progress";



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


    if(!parser.parse_file(
            manifest_path,
            manifest
        ))
    {
        std::cerr
            << parser.error()
            << '\n';


        return 1;
    }

    forge::BuildGraphValidator
        graph_validator;


    try
    {
        graph_validator.validate(
            manifest.graph()
        );
    }
    catch(const std::exception& exception)
    {
        std::cerr
            << manifest_path
            << ": invalid build graph: "
            << exception.what()
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


    const bool recovering =
        forge::FileSystem::exists(
            state_marker_path
        );


    if(recovering)
    {
        std::cout
            << "recovery: previous build state was not committed; "
            << "ignoring persisted logs\n";
    }
    else
    {
        const forge::LogLoadResult
            build_log_result =
                build_log.load(
                    build_log_path
                );


        const forge::LogLoadResult
            deps_log_result =
                deps_log.load(
                    deps_log_path
                );


        const bool both_loaded =
            build_log_result
                == forge::LogLoadResult::Ok
            && deps_log_result
                == forge::LogLoadResult::Ok;


        const bool fresh_build =
            build_log_result
                == forge::LogLoadResult::Missing
            && deps_log_result
                == forge::LogLoadResult::Missing;


        if(!both_loaded)
        {
            /*
             * BuildLog 和 DepsLog 共同描述上一轮构建状态。
             *
             * 只要其中任意一部分不可信，就不能继续使用
             * 另一部分做增量判断。
             */
            build_log.clear();
            deps_log.clear();


            if(!fresh_build)
            {
                std::cout
                    << "recovery: persisted state is not trustworthy; "
                    << "build_log="
                    << forge::log_load_result_name(
                        build_log_result
                    )
                    << ", deps_log="
                    << forge::log_load_result_name(
                        deps_log_result
                    )
                    << "; ignoring persisted logs\n";
            }
        }
    }


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

    if (options.explain)
    {
        for (const forge::Edge* edge :
             plan.edges())
        {
            std::cout
                << "[dirty] "
                << edge->describe()
                << '\n';


            for (const std::string& reason :
                 builder.reasons(edge))
            {
                std::cout
                    << "  - "
                    << reason
                    << '\n';
            }
        }
    }


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

    if(!forge::FileSystem::atomic_write_file(
            state_marker_path,
            "build state in progress\n"
        ))
    {
        std::cerr
            << "failed to create build state marker\n";

        return 1;
    }

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
    *
    * 构建成功后提交新的持久化状态。
    *
    * 提交顺序：
    *
    *   DepsLog
    *      ↓
    *   BuildLog
    *      ↓
    *   remove state marker
    *
    * marker 只有在两个日志都成功提交后才删除。
    *
    * 如果其中任何一步失败或进程异常退出，
    * marker 会保留下来，下一次启动将忽略旧日志并
    * 进行保守重建。
    */

    if(!deps_log.save(
            deps_log_path
        ))
    {
        std::cerr
            << "failed to save deps log\n";

        return 1;
    }


    if(!build_log.save(
            build_log_path
        ))
    {
        std::cerr
            << "failed to save build log\n";

        return 1;
    }


    std::error_code marker_error;


    std::filesystem::remove(
        state_marker_path,
        marker_error
    );


    if(marker_error)
    {
        std::cerr
            << "failed to commit build state marker\n";

        return 1;
    }


    std::cout
        << "build succeeded\n";


    return 0;
}