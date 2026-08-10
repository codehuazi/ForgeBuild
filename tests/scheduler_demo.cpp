#include "forge/scheduler.hpp"

#include "forge/manifest.hpp"
#include "forge/build_graph.hpp"
#include "forge/edge.hpp"
#include "forge/rule.hpp"
#include "forge/executor.hpp"

#include <iostream>
#include <vector>



int main()
{

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



    auto& graph =
        manifest.graph();

    auto* broken_source =
        graph.get_or_create_node(
            "broken.cpp"
        );


    auto* broken_object =
        graph.get_or_create_node(
            "broken.o"
        );


    auto* broken_compile =
        graph.create_edge(
            compile_rule
        );


    graph.add_input(
        broken_compile,
        broken_source
    );


    graph.add_output(
        broken_compile,
        broken_object
    );


    auto* source_main =
        graph.get_or_create_node(
            "main.cpp"
        );


    auto* object_main =
        graph.get_or_create_node(
            "main.o"
        );


    auto* source_a =
        graph.get_or_create_node(
            "a.cpp"
        );


    auto* object_a =
        graph.get_or_create_node(
            "a.o"
        );


    auto* source_b =
        graph.get_or_create_node(
            "b.cpp"
        );


    auto* object_b =
        graph.get_or_create_node(
            "b.o"
        );


    auto* app =
        graph.get_or_create_node(
            "app"
        );



    //
    // 创建 link
    //
    auto* link_edge =
        graph.create_edge(
            link_rule
        );


    graph.add_output(
        link_edge,
        app
    );



    //
    // 创建 compile
    //

    auto* compile_main =
        graph.create_edge(
            compile_rule
        );


    graph.add_input(
        compile_main,
        source_main
    );


    graph.add_output(
        compile_main,
        object_main
    );

    auto* compile_a =
        graph.create_edge(
            compile_rule
        );


    graph.add_input(
        compile_a,
        source_a
    );


    graph.add_output(
        compile_a,
        object_a
    );



    auto* compile_b =
        graph.create_edge(
            compile_rule
        );


    graph.add_input(
        compile_b,
        source_b
    );


    graph.add_output(
        compile_b,
        object_b
    );

    graph.add_input(
        link_edge,
        object_a
    );


    graph.add_input(
        link_edge,
        object_b
    );

    graph.add_input(
        link_edge,
        object_main
    );



    std::vector<
        forge::Edge*
    > edges;


    edges.push_back(
        compile_main
    );


    edges.push_back(
        compile_a
    );

    // edges.push_back(
    //     broken_compile
    // );


    edges.push_back(
        compile_b
    );


    edges.push_back(
        link_edge
    );



    forge::Scheduler scheduler(
        4
    );

    forge::Executor executor;


    scheduler.set_executor(
        &executor
    );


    bool result =
        scheduler.run(
            edges
        );


    std::cout
        << std::boolalpha
        << "scheduler result: "
        << result
        << "\n";


    return 0;
}