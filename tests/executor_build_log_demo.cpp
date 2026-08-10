#include "forge/build_log.hpp"
#include "forge/build_plan.hpp"
#include "forge/builder.hpp"
#include "forge/edge.hpp"
#include "forge/executor.hpp"
#include "forge/hash.hpp"
#include "forge/manifest.hpp"
#include "forge/node.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>


int main()
{
    // 从干净状态开始。
    std::system("rm -f main.o app");


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
    // 验证最终计划仍由拓扑排序决定。
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


    forge::BuildLog build_log;


    // 初始日志为空。
    assert(
        !build_log.contains(
            object->path()
        )
    );

    assert(
        !build_log.contains(
            executable->path()
        )
    );


    // Builder 同时检查：
    // 文件状态和命令日志。
    forge::Builder first_builder(
        graph,
        build_log
    );


    forge::BuildPlan first_plan =
        first_builder.build();


    std::cout
        << "first plan count: "
        << first_plan.edges().size()
        << '\n';


    assert(
        first_plan.edges().size()
        == 2
    );

    assert(
        first_plan.edges()[0]
        == compile_edge
    );

    assert(
        first_plan.edges()[1]
        == link_edge
    );


    // Executor 成功后应更新同一个 BuildLog。
    forge::Executor executor(
        build_log
    );


    const bool first_success =
        executor.execute(
            first_plan
        );


    assert(first_success);


    // 计算实际命令哈希。
    const std::uint64_t compile_hash =
        forge::hash_string(
            compile_edge->command()
        );

    const std::uint64_t link_hash =
        forge::hash_string(
            link_edge->command()
        );


    // 验证 Executor 已写入日志。
    assert(
        build_log.contains(
            object->path()
        )
    );

    assert(
        build_log.command_matches(
            object->path(),
            compile_hash
        )
    );


    assert(
        build_log.contains(
            executable->path()
        )
    );

    assert(
        build_log.command_matches(
            executable->path(),
            link_hash
        )
    );


    std::cout
        << "main.o hash recorded: "
        << build_log.command_hash(
            object->path()
        )
        << '\n';

    std::cout
        << "app hash recorded: "
        << build_log.command_hash(
            executable->path()
        )
        << '\n';


    // 再次规划。
    //
    // 此时：
    // 1. main.o 和 app 都存在
    // 2. 时间戳没有过期
    // 3. 命令日志完全匹配
    //
    // 所以计划应为空。
    forge::Builder second_builder(
        graph,
        build_log
    );


    forge::BuildPlan second_plan =
        second_builder.build();


    std::cout
        << "second plan count: "
        << second_plan.edges().size()
        << '\n';


    assert(
        second_plan.edges().empty()
    );


    std::cout
        << "executor build log checks passed\n";


    return 0;
}