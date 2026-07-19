#include "forge/build_graph.hpp"
#include "forge/edge.hpp"
#include "forge/node.hpp"
#include "forge/rule.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

int main()
{
    // Rule 必须比 Graph 中引用它们的 Edge 活得更久。
    forge::Rule compile_rule(
        "compile",
        "g++ -c $in -o $out"
    );

    forge::Rule link_rule(
        "link",
        "g++ $in -o $out"
    );

    forge::BuildGraph graph;

    // 创建5个唯一的文件节点。
    auto* main_cpp =
        graph.get_or_create_node("main.cpp");

    auto* math_cpp =
        graph.get_or_create_node("math.cpp");

    auto* main_o =
        graph.get_or_create_node("main.o");

    auto* math_o =
        graph.get_or_create_node("math.o");

    auto* app =
        graph.get_or_create_node("app");

    // main.cpp --compile--> main.o
    auto* compile_main =
        graph.create_edge(&compile_rule);

    graph.add_input(compile_main, main_cpp);
    graph.add_output(compile_main, main_o);

    // math.cpp --compile--> math.o
    auto* compile_math =
        graph.create_edge(&compile_rule);

    graph.add_input(compile_math, math_cpp);
    graph.add_output(compile_math, math_o);

    // main.o + math.o --link--> app
    auto* link_app =
        graph.create_edge(&link_rule);

    graph.add_input(link_app, main_o);
    graph.add_input(link_app, math_o);
    graph.add_output(link_app, app);

    // 验证同一路径只对应一个Node。
    auto* main_cpp_again =
        graph.get_or_create_node("main.cpp");

    assert(main_cpp_again == main_cpp);

    // 验证main.cpp -> main.o。
    assert(compile_main->inputs().size() == 1);
    assert(compile_main->inputs().at(0) == main_cpp);

    assert(compile_main->outputs().size() == 1);
    assert(compile_main->outputs().at(0) == main_o);

    assert(main_cpp->in_edge() == nullptr);
    assert(main_cpp->out_edges().size() == 1);
    assert(main_cpp->out_edges().at(0) == compile_main);

    // 验证math.cpp -> math.o。
    assert(compile_math->inputs().size() == 1);
    assert(compile_math->inputs().at(0) == math_cpp);

    assert(compile_math->outputs().size() == 1);
    assert(compile_math->outputs().at(0) == math_o);

    assert(math_cpp->in_edge() == nullptr);
    assert(math_cpp->out_edges().size() == 1);
    assert(math_cpp->out_edges().at(0) == compile_math);

    // 验证main.o和math.o连接到link Edge。
    assert(main_o->in_edge() == compile_main);
    assert(main_o->out_edges().size() == 1);
    assert(main_o->out_edges().at(0) == link_app);

    assert(math_o->in_edge() == compile_math);
    assert(math_o->out_edges().size() == 1);
    assert(math_o->out_edges().at(0) == link_app);

    // 验证main.o + math.o -> app。
    assert(link_app->inputs().size() == 2);
    assert(link_app->inputs().at(0) == main_o);
    assert(link_app->inputs().at(1) == math_o);

    assert(link_app->outputs().size() == 1);
    assert(link_app->outputs().at(0) == app);

    assert(app->in_edge() == link_app);
    assert(app->out_edges().empty());

    std::cout << "Build graph:\n";
    std::cout << "  main.cpp --compile--> main.o\n";
    std::cout << "  math.cpp --compile--> math.o\n";
    std::cout << "  main.o + math.o --link--> app\n";

    std::cout
        << "Node uniqueness: "
        << std::boolalpha
        << (main_cpp == main_cpp_again)
        << '\n';

    std::cout << "All graph checks passed.\n";

    return 0;
}