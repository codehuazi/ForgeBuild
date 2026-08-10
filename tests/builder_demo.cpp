#include "forge/manifest.hpp"
#include "forge/builder.hpp"
#include "forge/node.hpp"
#include "forge/edge.hpp"


#include <iostream>
#include <cassert>


int main()
{

    forge::Manifest manifest;


    auto* link =
        manifest.add_rule(
            "link",
            "g++"
        );

    auto* compile =
        manifest.add_rule(
            "compile",
            "g++ -c"
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

    auto* app =
        graph.get_or_create_node(
            "app"
        );


    auto* compile_main =
        graph.create_edge(
            compile
        );


    graph.add_input(
        compile_main,
        cpp
    );


    graph.add_output(
        compile_main,
        obj
    );

    auto* link_app =
        graph.create_edge(
            link
        );


    graph.add_input(
        link_app,
        obj
    );


    graph.add_output(
        link_app,
        app
    );


    cpp->mark_dirty();


    forge::Builder builder(graph);

    auto plan =
        builder.build();

    plan.print();

    std::cout
        << "Plan edges: "
        << plan.edges().size()
        << "\n";

    std::cout
        << "main.o dirty: "
        << obj->dirty()
        << "\n";

    assert(
        plan.edges().size()==2
    );


    assert(
        link_app->depends_on(
            compile_main
        )
    );

    assert(
        !compile_main->depends_on(
            link_app
        )
    );




}
