#include "forge/build_graph.hpp"
#include "forge/build_log.hpp"
#include "forge/build_plan.hpp"
#include "forge/builder.hpp"
#include "forge/deps_log.hpp"
#include "forge/edge.hpp"
#include "forge/hash.hpp"
#include "forge/manifest.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>


namespace fs = std::filesystem;


namespace
{


void create_file(
    const std::string& path
)
{
    std::ofstream output(path);

    output
        << "demo file\n";
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

    for(const auto* edge : plan.edges())
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
        "dynamic_dependency_demo_data";

    const std::string source_path =
        directory + "/main.cpp";

    const std::string header_path =
        directory + "/config.h";

    const std::string object_path =
        directory + "/main.o";

    const std::string app_path =
        directory + "/app";


    fs::remove_all(directory);

    fs::create_directories(directory);


    create_file(source_path);
    create_file(header_path);
    create_file(object_path);
    create_file(app_path);


    /*
     * 设置一组确定的时间关系：
     *
     * main.cpp、config.h
     *          ↓
     *        main.o
     *          ↓
     *          app
     *
     * 初始情况下：
     *
     * main.cpp  = now - 30秒
     * config.h  = now - 30秒
     * main.o    = now - 20秒
     * app       = now - 10秒
     *
     * 所以所有输出都比输入新，
     * 第一次构建计划应该为空。
     */

    const auto now =
        fs::file_time_type::clock::now();


    fs::last_write_time(
        source_path,
        now - std::chrono::seconds(30)
    );

    fs::last_write_time(
        header_path,
        now - std::chrono::seconds(30)
    );

    fs::last_write_time(
        object_path,
        now - std::chrono::seconds(20)
    );

    fs::last_write_time(
        app_path,
        now - std::chrono::seconds(10)
    );


    forge::Manifest manifest;


    auto* compile_rule =
        manifest.add_rule(
            "compile",
            "g++ -c $in -o $out"
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


    /*
     * 故意先创建 link Edge，
     * 验证 BuildPlanner 仍能恢复正确顺序。
     */

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


    /*
     * 模拟上一次构建留下的命令记录。
     *
     * 否则空的 BuildLog 会认为命令没有历史记录，
     * 从而把两个 Edge 都判断为需要构建。
     */

    forge::BuildLog build_log;


    build_log.record(
        object_path,
        forge::hash_string(
            compile_edge->command()
        )
    );


    build_log.record(
        app_path,
        forge::hash_string(
            link_edge->command()
        )
    );


    /*
     * 模拟从 main.d 中得到的动态依赖：
     *
     * main.o:
     *     main.cpp
     *     config.h
     */

    forge::DepsLog deps_log;


    deps_log.record(
        object_path,
        {
            source_path,
            header_path
        }
    );


    /*
     * 场景1：
     *
     * 所有输出都比输入新。
     *
     * 预期：
     * plan edge count = 0
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
            "case 1: everything is clean",
            plan
        );
    }


    /*
     * 场景2：
     *
     * config.h 比 main.o 更新。
     *
     * 预期：
     *
     * compile Edge 被动态依赖判断为 dirty；
     * link Edge 因为依赖 main.o 而被向下游传播。
     *
     * plan edge count = 2
     */

    fs::last_write_time(
        header_path,
        now
    );


    {
        forge::Builder builder(
            graph,
            build_log,
            deps_log
        );

        forge::BuildPlan plan =
            builder.build();

        print_plan(
            "case 2: config.h is newer",
            plan
        );
    }


    /*
     * 场景3：
     *
     * 删除动态依赖 config.h。
     *
     * 动态依赖文件不存在时，
     * 旧的 main.o 也不能被认为可靠。
     *
     * 预期：
     * plan edge count = 2
     */

    fs::remove(header_path);


    {
        forge::Builder builder(
            graph,
            build_log,
            deps_log
        );

        forge::BuildPlan plan =
            builder.build();

        print_plan(
            "case 3: config.h is missing",
            plan
        );
    }


    fs::remove_all(directory);


    return 0;
}