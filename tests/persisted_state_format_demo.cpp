#include "forge/build_log.hpp"
#include "forge/deps_log.hpp"
#include "forge/log_load_result.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


namespace
{


bool write_file(
    const std::string& path,
    const std::string& content
)
{
    std::ofstream output(
        path,
        std::ios::binary
        | std::ios::trunc
    );


    if(!output)
    {
        return false;
    }


    output
        << content;


    output.close();


    return static_cast<bool>(
        output
    );
}


}


int main()
{
    namespace fs =
        std::filesystem;


    const std::string root =
        ".forge_persisted_state_format_test";


    const std::string build_path =
        root + "/build.log";


    const std::string deps_path =
        root + "/deps.log";


    fs::remove_all(
        root
    );


    fs::create_directory(
        root
    );


    /*
     * 1. 文件不存在。
     */
    {
        forge::BuildLog build_log;
        forge::DepsLog deps_log;


        if(build_log.load(
                build_path
            )
                != forge::LogLoadResult::Missing
            || deps_log.load(
                deps_path
            )
                != forge::LogLoadResult::Missing)
        {
            std::cerr
                << "missing logs were not classified correctly\n";

            fs::remove_all(
                root
            );

            return 1;
        }
    }


    /*
     * 2. 正常保存和重新加载。
     */
    {
        forge::BuildLog build_log;
        forge::DepsLog deps_log;


        build_log.record(
            "a.o",
            123
        );


        deps_log.record(
            "a.o",
            {
                "a.cpp",
                "a.hpp"
            }
        );


        if(!build_log.save(
                build_path
            )
            || !deps_log.save(
                deps_path
            ))
        {
            std::cerr
                << "failed to save versioned logs\n";

            fs::remove_all(
                root
            );

            return 1;
        }


        forge::BuildLog loaded_build;
        forge::DepsLog loaded_deps;


        if(loaded_build.load(
                build_path
            )
                != forge::LogLoadResult::Ok
            || loaded_deps.load(
                deps_path
            )
                != forge::LogLoadResult::Ok)
        {
            std::cerr
                << "failed to load versioned logs\n";

            fs::remove_all(
                root
            );

            return 1;
        }


        if(loaded_build.command_hash(
                "a.o"
            ) != 123)
        {
            std::cerr
                << "build log value was not preserved\n";

            fs::remove_all(
                root
            );

            return 1;
        }


        const std::vector<std::string>
            expected_dependencies{
                "a.cpp",
                "a.hpp"
            };


        if(loaded_deps.inputs(
                "a.o"
            )
            != expected_dependencies)
        {
            std::cerr
                << "deps log value was not preserved\n";

            fs::remove_all(
                root
            );

            return 1;
        }
    }


    /*
     * 3. 不支持的版本。
     */
    if(!write_file(
            build_path,
            "FORGEBUILD_BUILD_LOG_V999\n"
        )
        || !write_file(
            deps_path,
            "FORGEBUILD_DEPS_LOG_V999\n"
        ))
    {
        std::cerr
            << "failed to prepare unsupported-version logs\n";

        fs::remove_all(
            root
        );

        return 1;
    }


    {
        forge::BuildLog build_log;
        forge::DepsLog deps_log;


        if(build_log.load(
                build_path
            )
                != forge::LogLoadResult::UnsupportedVersion
            || deps_log.load(
                deps_path
            )
                != forge::LogLoadResult::UnsupportedVersion)
        {
            std::cerr
                << "unsupported versions were not classified correctly\n";

            fs::remove_all(
                root
            );

            return 1;
        }
    }


    /*
     * 4. Header 正常但正文损坏。
     *
     * 同时验证 load() 失败不能破坏对象原来的状态。
     */
    if(!write_file(
            build_path,
            "FORGEBUILD_BUILD_LOG_V1\n"
            "broken-line-without-tab\n"
        )
        || !write_file(
            deps_path,
            "FORGEBUILD_DEPS_LOG_V1\n"
            "a.o\n"
            "not-a-number\n"
        ))
    {
        std::cerr
            << "failed to prepare corrupted logs\n";

        fs::remove_all(
            root
        );

        return 1;
    }


    forge::BuildLog stable_build;
    forge::DepsLog stable_deps;


    stable_build.record(
        "stable.o",
        456
    );


    stable_deps.record(
        "stable.o",
        {
            "stable.cpp"
        }
    );


    if(stable_build.load(
            build_path
        )
            != forge::LogLoadResult::Corrupted
        || stable_deps.load(
            deps_path
        )
            != forge::LogLoadResult::Corrupted)
    {
        std::cerr
            << "corrupted logs were not classified correctly\n";

        fs::remove_all(
            root
        );

        return 1;
    }


    if(stable_build.command_hash(
            "stable.o"
        ) != 456)
    {
        std::cerr
            << "failed build-log load corrupted existing state\n";

        fs::remove_all(
            root
        );

        return 1;
    }


    const std::vector<std::string>
        expected_stable_dependencies{
            "stable.cpp"
        };


    if(stable_deps.inputs(
            "stable.o"
        )
        != expected_stable_dependencies)
    {
        std::cerr
            << "failed deps-log load corrupted existing state\n";

        fs::remove_all(
            root
        );

        return 1;
    }


    fs::remove_all(
        root
    );


    std::cout
        << "persisted state format checks passed\n";


    return 0;
}