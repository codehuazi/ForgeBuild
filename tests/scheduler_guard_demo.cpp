#include "forge/build_graph.hpp"
#include "forge/edge.hpp"
#include "forge/executor.hpp"
#include "forge/manifest.hpp"
#include "forge/scheduler.hpp"

#include <iostream>
#include <vector>


int main()
{
    forge::Manifest manifest;


    auto* rule =
        manifest.add_rule(
            "demo",
            "echo scheduler guard demo"
        );


    forge::BuildGraph& graph =
        manifest.graph();


    auto* output =
        graph.get_or_create_node(
            "guard_output.txt"
        );


    auto* edge =
        graph.create_edge(
            rule
        );


    graph.add_output(
        edge,
        output
    );


    std::vector<forge::Edge*> edges{
        edge
    };


    /*
     * 场景1：
     * jobs == 0。
     *
     * Scheduler 应立即返回 false，
     * 而不是创建零个 Worker 后永久等待。
     */

    forge::Executor executor;


    forge::Scheduler zero_jobs_scheduler(
        0
    );


    zero_jobs_scheduler.set_executor(
        &executor
    );


    const bool zero_jobs_result =
        zero_jobs_scheduler.run(
            edges
        );


    std::cout
        << std::boolalpha
        << "zero jobs result: "
        << zero_jobs_result
        << '\n';


    /*
     * 场景2：
     * jobs 合法，但没有设置 Executor。
     *
     * Scheduler 应立即返回 false，
     * 不能把任务假装执行成功。
     */

    forge::Scheduler missing_executor_scheduler(
        2
    );


    const bool missing_executor_result =
        missing_executor_scheduler.run(
            edges
        );


    std::cout
        << std::boolalpha
        << "missing executor result: "
        << missing_executor_result
        << '\n';


    /*
     * 两个非法场景都必须失败。
     */

    if (zero_jobs_result
        || missing_executor_result)
    {
        std::cerr
            << "scheduler guard test failed\n";

        return 1;
    }


    std::cout
        << "scheduler guard test passed\n";


    return 0;
}