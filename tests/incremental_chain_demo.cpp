#include "forge/build_log.hpp"
#include "forge/build_plan.hpp"
#include "forge/builder.hpp"
#include "forge/edge.hpp"
#include "forge/executor.hpp"
#include "forge/manifest.hpp"

#include <iostream>
#include <string>

int main()
{
    // Manifest 同时管理规则和构建图。
    forge::Manifest manifest;


    // 编译规则：
    //
    // main.cpp -> main.o
    auto* compile_rule =
        manifest.add_rule(
            "compile",
            "g++ -c $in -o $out"
        );


    // 链接规则：
    //
    // main.o -> app
    auto* link_rule =
        manifest.add_rule(
            "link",
            "g++ $in -o $out"
        );


    forge::BuildGraph& graph =
        manifest.graph();


    // 创建三个构建节点。
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


    // 故意先创建 link Edge。
    //
    // 这样 graph_.edges() 中很可能是：
    //
    // link
    // compile
    //
    // 我们要验证 BuildPlanner 最后仍然能输出：
    //
    // compile
    // link
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


    // 后创建 compile Edge。
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


    // Builder 完成：
    //
    // 1. 刷新文件状态
    // 2. 判断需要构建的 Edge
    // 3. 向下游传播
    // 4. 拓扑排序
    // 5. 生成 BuildPlan
    const std::string build_log_path =
        ".forge_log";


    forge::BuildLog build_log;


    const forge::LogLoadResult load_result =
        build_log.load(
            build_log_path
        );


    if(load_result
            != forge::LogLoadResult::Ok
        && load_result
            != forge::LogLoadResult::Missing)
    {
        std::cerr
            << "failed to load build log: "
            << forge::log_load_result_name(
                load_result
            )
            << '\n';

        return 1;
    }


    forge::Builder builder(
        graph,
        build_log
    );

    forge::BuildPlan plan =
        builder.build();


    std::cout
        << "plan edge count: "
        << plan.edges().size()
        << '\n';


    // 在真正执行前，先打印计划中的命令。
    //
    // 这样可以直接观察拓扑顺序。
    std::cout
        << "planned commands:\n";

    for (const auto* edge : plan.edges())
    {
        std::cout
            << "  "
            << edge->command()
            << '\n';
    }


    forge::Executor executor(
        build_log
    );

    bool success =
        executor.execute(plan);


    if (!success)
    {
        std::cout
            << std::boolalpha
            << "execute result: "
            << success
            << '\n';

        return 1;
    }


    if (!build_log.save(
            build_log_path
        ))
    {
        std::cerr
            << "failed to save build log\n";

        return 1;
    }


    std::cout
        << std::boolalpha
        << "execute result: "
        << success
        << '\n';


    return 0;
}