#include "forge/build_graph.hpp"
#include "forge/build_planner.hpp"
#include "forge/edge.hpp"
#include "forge/manifest.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>


int main()
{
    forge::Manifest manifest;

    auto* generate_rule =
        manifest.add_rule(
            "generate",
            "generate"
        );

    auto* consume_rule =
        manifest.add_rule(
            "consume",
            "consume"
        );


    auto& graph =
        manifest.graph();


    auto* generated_cpp =
        graph.get_or_create_node(
            "generated.cpp"
        );

    auto* generated_hpp =
        graph.get_or_create_node(
            "generated.hpp"
        );

    auto* result =
        graph.get_or_create_node(
            "result.txt"
        );


    //
    // 一个 producer Edge 同时生成两个 Output。
    //
    auto* generate_edge =
        graph.create_edge(
            generate_rule
        );

    graph.add_output(
        generate_edge,
        generated_cpp
    );

    graph.add_output(
        generate_edge,
        generated_hpp
    );


    //
    // consumer 同时依赖这两个 Output。
    //
    // 从任务依赖角度看：
    //
    //   generate -> consume
    //
    // 只有一条 producer dependency，
    // 而不是两条。
    //
    auto* consume_edge =
        graph.create_edge(
            consume_rule
        );

    graph.add_input(
        consume_edge,
        generated_cpp
    );

    graph.add_input(
        consume_edge,
        generated_hpp
    );

    graph.add_output(
        consume_edge,
        result
    );


    std::vector<forge::Edge*> edges{
        generate_edge,
        consume_edge
    };


    forge::BuildPlanner planner;

    std::vector<forge::Edge*> plan;


    try
    {
        plan =
            planner.plan(
                edges
            );
    }
    catch(const std::exception& exception)
    {
        std::cerr
            << "build planner unexpectedly failed: "
            << exception.what()
            << '\n';

        return 1;
    }


    if(plan.size() != 2)
    {
        std::cerr
            << "expected 2 planned edges, got "
            << plan.size()
            << '\n';

        return 1;
    }


    if(plan[0] != generate_edge)
    {
        std::cerr
            << "generate edge must run before consume edge\n";

        return 1;
    }


    if(plan[1] != consume_edge)
    {
        std::cerr
            << "consume edge must run after generate edge\n";

        return 1;
    }


    std::cout
        << "build planner multi-output test passed\n";


    return 0;
}