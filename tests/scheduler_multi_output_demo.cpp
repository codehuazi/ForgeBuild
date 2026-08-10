#include "forge/build_graph.hpp"
#include "forge/edge.hpp"
#include "forge/executor.hpp"
#include "forge/manifest.hpp"
#include "forge/scheduler.hpp"

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <string>


int main()
{
    forge::Manifest manifest;

    namespace fs =
        std::filesystem;


    fs::remove(
        "generated.cpp"
    );


    fs::remove(
        "generated.hpp"
    );


    fs::remove(
        "result.txt"
    );


    auto* generate_rule =
        manifest.add_rule(
            "generate",
            "sh -c 'echo generated-header > generated.hpp && "
            "echo generated-source > generated.cpp'"
        );


    auto* consume_rule =
        manifest.add_rule(
            "consume",
            "sh -c 'test -f generated.cpp && "
            "test -f generated.hpp && "
            "echo consumed > result.txt'"
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


    bool files_exist =
        fs::exists(
            "generated.cpp"
        )
        && fs::exists(
            "generated.hpp"
        )
        && fs::exists(
            "result.txt"
        );


    std::string result_content;


    if (files_exist)
    {
        std::ifstream input(
            "result.txt"
        );


        std::getline(
            input,
            result_content
        );
    }


    const bool content_correct =
        result_content == "consumed";


    const bool test_passed =
        scheduler_result
        && files_exist
        && content_correct;


    std::cout
        << std::boolalpha
        << "scheduler result: "
        << scheduler_result
        << '\n'
        << "files exist: "
        << files_exist
        << '\n'
        << "content correct: "
        << content_correct
        << '\n';


    if (!test_passed)
    {
        std::cerr
            << "scheduler multi-output test failed\n";

        return 1;
    }


    std::cout
        << "scheduler multi-output test passed\n";


    return 0;
}