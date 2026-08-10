#include "forge/build_graph.hpp"
#include "forge/edge.hpp"
#include "forge/executor.hpp"
#include "forge/manifest.hpp"
#include "forge/scheduler.hpp"

#include <filesystem>
#include <iostream>
#include <vector>


int main()
{
    namespace fs =
        std::filesystem;


    fs::remove(
        "failure_output.txt"
    );


    fs::remove(
        "downstream_output.txt"
    );


    forge::Manifest manifest;


    /*
     * 这个命令故意失败。
     *
     * 它先删除输出文件，再返回非零退出码，
     * 避免旧文件干扰测试。
     */
    auto* failing_rule =
        manifest.add_rule(
            "failing",
            "sh -c 'rm -f failure_output.txt && exit 1'"
        );


    /*
     * 这个下游任务绝对不应该被执行。
     */
    auto* downstream_rule =
        manifest.add_rule(
            "downstream",
            "sh -c 'echo should-not-run > downstream_output.txt'"
        );


    auto& graph =
        manifest.graph();


    auto* failure_output =
        graph.get_or_create_node(
            "failure_output.txt"
        );


    auto* downstream_output =
        graph.get_or_create_node(
            "downstream_output.txt"
        );


    auto* failing_edge =
        graph.create_edge(
            failing_rule
        );


    graph.add_output(
        failing_edge,
        failure_output
    );


    auto* downstream_edge =
        graph.create_edge(
            downstream_rule
        );


    graph.add_input(
        downstream_edge,
        failure_output
    );


    graph.add_output(
        downstream_edge,
        downstream_output
    );


    std::vector<forge::Edge*> edges{
        failing_edge,
        downstream_edge
    };


    forge::Executor executor;


    forge::Scheduler scheduler(
        2
    );


    scheduler.set_executor(
        &executor
    );


    const bool scheduler_result =
        scheduler.run(
            edges
        );


    const bool downstream_exists =
        fs::exists(
            "downstream_output.txt"
        );


    /*
     * 正确结果：
     *
     * Scheduler 应返回 false，
     * 并且下游文件不应存在。
     */
    const bool test_passed =
        !scheduler_result
        && !downstream_exists;


    std::cout
        << std::boolalpha
        << "scheduler result: "
        << scheduler_result
        << '\n'
        << "downstream exists: "
        << downstream_exists
        << '\n';


    if (!test_passed)
    {
        std::cerr
            << "scheduler failure propagation test failed\n";

        return 1;
    }


    std::cout
        << "scheduler failure propagation test passed\n";


    return 0;
}