#include "forge/parser.hpp"
#include "forge/manifest.hpp"
#include "forge/rule.hpp"

#include <cassert>
#include <iostream>


int main()
{

    forge::Manifest manifest;


    forge::Parser parser;


    bool result =
        parser.parse_file(
            "build.forge",
            manifest
        );


    assert(result);


    auto* rule =
        manifest.find_rule(
            "compile"
        );


    assert(rule != nullptr);

    assert(
        rule->command()
        ==
        "g++ -MMD -MF $out.d -c $in -o $out"
    );

    assert(
        rule->depfile()
        ==
        "$out.d"
    );

    std::cout
        << "command: "
        << rule->command()
        << '\n';

    std::cout
        << "depfile: "
        << rule->depfile()
        << '\n';

    manifest.graph()
        .dump();

    std::cout
        << "parser passed\n";

}