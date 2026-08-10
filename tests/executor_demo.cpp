
#include "forge/manifest.hpp"
#include "forge/builder.hpp"
#include "forge/executor.hpp"
#include "forge/node.hpp"


#include <iostream>


int main()
{

    forge::Manifest manifest;


    auto* compile =
        manifest.add_rule(
            "compile",
            "g++ -c $in -o $out"
        );


    auto& graph =
        manifest.graph();


    auto* cpp =
        graph.get_or_create_node(
            "main.cpp"
        );


    auto* obj =
        graph.get_or_create_node(
            "main.o"
        );


    auto* edge =
        graph.create_edge(
            compile
        );


    graph.add_input(
        edge,
        cpp
    );


    graph.add_output(
        edge,
        obj
    );


    forge::Builder builder(graph);


    auto plan =
        builder.build();

    std::cout
        << "plan edge count: "
        << plan.edges().size()
        << '\n';

    forge::Executor executor;


    bool result =
        executor.execute(plan);



    std::cout
        << "execute result: "
        << std::boolalpha
        << result
        << "\n";

    std::cout
        << "main.o exists: "
        << obj->exists()
        << "\n";


    std::cout
        << "main.o dirty: "
        << obj->dirty()
        << "\n";

}