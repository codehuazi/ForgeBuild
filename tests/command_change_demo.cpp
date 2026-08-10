#include "forge/build_log.hpp"
#include "forge/build_plan.hpp"
#include "forge/builder.hpp"
#include "forge/node.hpp"
#include "forge/edge.hpp"
#include "forge/hash.hpp"
#include "forge/manifest.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>


int main()
{
    // ------------------------------------------------------------
    // 测试准备：
    // 先真实生成 main.o 和 app。
    //
    // 这样后续 Builder 检查时：
    // 1. 输出文件都存在
    // 2. 输出时间戳也是最新的
    //
    // 因此后面产生的重新构建计划，
    // 才能确定是由 BuildLog 命令检查触发的。
    // ------------------------------------------------------------

    int compile_result =
        std::system(
            "g++ -c main.cpp -o main.o"
        );

    assert(compile_result == 0);


    int link_result =
        std::system(
            "g++ main.o -o app"
        );

    assert(link_result == 0);


    // ============================================================
    // 第一组构建图：
    //
    // compile:
    // g++ -c main.cpp -o main.o
    //
    // link:
    // g++ main.o -o app
    // ============================================================

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
            "main.cpp"
        );

    auto* object =
        graph.get_or_create_node(
            "main.o"
        );

    auto* executable =
        graph.get_or_create_node(
            "app"
        );


    // 故意先创建 link Edge，
    // 继续验证拓扑排序不依赖创建顺序。
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
        executable
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


    // ============================================================
    // 场景一：BuildLog 为空
    // ============================================================

    forge::BuildLog empty_log;


    forge::Builder first_builder(
        graph,
        empty_log
    );


    forge::BuildPlan first_plan =
        first_builder.build();


    std::cout
        << "empty log plan count: "
        << first_plan.edges().size()
        << '\n';


    for (const auto* edge :
         first_plan.edges())
    {
        std::cout
            << "  "
            << edge->command()
            << '\n';
    }


    // 虽然文件都存在且最新，
    // 但日志中没有命令记录。
    //
    // compile 和 link 都应该重新执行。
    assert(
        first_plan.edges().size()
        == 2
    );


    // ============================================================
    // 场景二：记录当前命令
    // ============================================================

    forge::BuildLog matching_log;


    const std::uint64_t compile_hash =
        forge::hash_string(
            compile_edge->command()
        );

    const std::uint64_t link_hash =
        forge::hash_string(
            link_edge->command()
        );


    matching_log.record(
        object->path(),
        compile_hash
    );

    matching_log.record(
        executable->path(),
        link_hash
    );


    forge::Builder second_builder(
        graph,
        matching_log
    );


    forge::BuildPlan second_plan =
        second_builder.build();


    std::cout
        << "matching log plan count: "
        << second_plan.edges().size()
        << '\n';


    // 文件状态没有变化，
    // 命令记录也完全匹配。
    //
    // 因此不应该执行任何 Edge。
    assert(
        second_plan.edges().empty()
    );


    // ============================================================
    // 场景三：compile 命令增加 -O2
    //
    // 重新创建一张构建图，
    // 但继续使用旧的 matching_log。
    // ============================================================

    forge::Manifest changed_manifest;


    auto* changed_compile_rule =
        changed_manifest.add_rule(
            "compile",
            "g++ -O2 -c $in -o $out"
        );


    auto* unchanged_link_rule =
        changed_manifest.add_rule(
            "link",
            "g++ $in -o $out"
        );


    forge::BuildGraph& changed_graph =
        changed_manifest.graph();


    auto* changed_source =
        changed_graph.get_or_create_node(
            "main.cpp"
        );

    auto* changed_object =
        changed_graph.get_or_create_node(
            "main.o"
        );

    auto* changed_executable =
        changed_graph.get_or_create_node(
            "app"
        );


    auto* changed_link_edge =
        changed_graph.create_edge(
            unchanged_link_rule
        );

    changed_graph.add_input(
        changed_link_edge,
        changed_object
    );

    changed_graph.add_output(
        changed_link_edge,
        changed_executable
    );


    auto* changed_compile_edge =
        changed_graph.create_edge(
            changed_compile_rule
        );

    changed_graph.add_input(
        changed_compile_edge,
        changed_source
    );

    changed_graph.add_output(
        changed_compile_edge,
        changed_object
    );


    forge::Builder third_builder(
        changed_graph,
        matching_log
    );


    forge::BuildPlan third_plan =
        third_builder.build();


    std::cout
        << "changed command plan count: "
        << third_plan.edges().size()
        << '\n';


    for (const auto* edge :
         third_plan.edges())
    {
        std::cout
            << "  "
            << edge->command()
            << '\n';
    }


    // compile 命令发生变化：
    //
    // g++ -c ...
    //      ↓
    // g++ -O2 -c ...
    //
    // compile Edge 需要重建。
    //
    // main.o 将发生变化，
    // 所以 link Edge 也必须进入计划。
    assert(
        third_plan.edges().size()
        == 2
    );


    assert(
        third_plan.edges()[0]
        == changed_compile_edge
    );

    assert(
        third_plan.edges()[1]
        == changed_link_edge
    );


    std::cout
        << "command change checks passed\n";


    return 0;
}