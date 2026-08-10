#include "forge/build_graph.hpp"
#include "forge/build_log.hpp"
#include "forge/build_plan.hpp"
#include "forge/builder.hpp"
#include "forge/deps_log.hpp"
#include "forge/executor.hpp"
#include "forge/manifest.hpp"
#include "forge/rule.hpp"
#include "forge/edge.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>


namespace fs = std::filesystem;


namespace
{


void write_source_files(
    const std::string& directory
)
{
    std::ofstream header(
        directory + "/config.h"
    );

    header
        << "#pragma once\n"
        << "#define VALUE 42\n";


    std::ofstream source(
        directory + "/main.cpp"
    );

    source
        << "#include \"config.h\"\n"
        << "#include <iostream>\n"
        << "\n"
        << "int main()\n"
        << "{\n"
        << "    std::cout << VALUE << '\\n';\n"
        << "    return 0;\n"
        << "}\n";
}


void print_plan(
    const std::string& title,
    const forge::BuildPlan& plan
)
{
    std::cout
        << "\n"
        << title
        << "\n";

    std::cout
        << "plan edge count: "
        << plan.edges().size()
        << "\n";

    for (const forge::Edge* edge :
         plan.edges())
    {
        std::cout
            << "  "
            << edge->command()
            << "\n";
    }
}


}


int main()
{
    const std::string directory =
        "real_depfile_demo_data";

    const std::string source_path =
        directory + "/main.cpp";

    const std::string header_path =
        directory + "/config.h";

    const std::string object_path =
        directory + "/main.o";

    const std::string depfile_path =
        directory + "/main.o.d";

    const std::string app_path =
        directory + "/app";

    const std::string build_log_path =
        directory + "/.forge_log";

    const std::string deps_log_path =
        directory + "/.forge_deps";


    fs::remove_all(directory);

    fs::create_directories(directory);

    write_source_files(directory);


    forge::Manifest manifest;


    auto* compile_rule =
        manifest.add_rule(
            "compile",
            "g++ -std=c++20 -MMD -MF "
            + depfile_path
            + " -c $in -o $out"
        );


    compile_rule->set_depfile(
        depfile_path
    );


    auto* link_rule =
        manifest.add_rule(
            "link",
            "g++ $in -o $out"
        );


    forge::BuildGraph& graph =
        manifest.graph();


    auto* source =
        graph.get_or_create_node(
            source_path
        );

    auto* object =
        graph.get_or_create_node(
            object_path
        );

    auto* app =
        graph.get_or_create_node(
            app_path
        );


    auto* link_edge =
        graph.create_edge(
            link_rule
        );

    graph.add_input(
        link_edge,
        object
    );

    graph.add_output(
        link_edge,
        app
    );


    auto* compile_edge =
        graph.create_edge(
            compile_rule
        );

    graph.add_input(
        compile_edge,
        source
    );

    graph.add_output(
        compile_edge,
        object
    );


    forge::BuildLog build_log;

    forge::DepsLog deps_log;


    /*
     * 第一次构建：
     * 输出都不存在，所以应该编译和链接。
     */

    {
        forge::Builder builder(
            graph,
            build_log,
            deps_log
        );

        forge::BuildPlan plan =
            builder.build();

        print_plan(
            "case 1: first build",
            plan
        );


        forge::Executor executor(
            build_log,
            deps_log
        );


        if (!executor.execute(plan))
        {
            std::cerr
                << "first build failed\n";

            return 1;
        }
    }


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


    /*
     * 创建全新的日志对象，模拟程序重启。
     */

    forge::BuildLog loaded_build_log;

    forge::DepsLog loaded_deps_log;


    if (!loaded_build_log.load(
            build_log_path
        ))
    {
        std::cerr
            << "failed to load build log\n";

        return 1;
    }


    if (!loaded_deps_log.load(
            deps_log_path
        ))
    {
        std::cerr
            << "failed to load deps log\n";

        return 1;
    }


    /*
     * 第二次构建：
     * 没有任何修改，计划应该为空。
     */

    {
        forge::Builder builder(
            graph,
            loaded_build_log,
            loaded_deps_log
        );

        forge::BuildPlan plan =
            builder.build();

        print_plan(
            "case 2: no changes",
            plan
        );
    }


    /*
     * 修改头文件内容。
     *
     * Builder 将通过 loaded_deps_log 得知：
     *
     * main.o 依赖 config.h。
     */

    {
        std::ofstream header(
            header_path
        );

        header
            << "#pragma once\n"
            << "#define VALUE 100\n";
    }


    /*
     * 某些文件系统时间戳精度较低。
     * 这里手动将头文件时间设置到未来一秒，
     * 保证它一定比 main.o 更新。
     */

    fs::last_write_time(
        header_path,
        fs::file_time_type::clock::now()
            + std::chrono::seconds(1)
    );


    {
        forge::Builder builder(
            graph,
            loaded_build_log,
            loaded_deps_log
        );

        forge::BuildPlan plan =
            builder.build();

        print_plan(
            "case 3: config.h changed",
            plan
        );
    }


    fs::remove_all(directory);


    return 0;
}