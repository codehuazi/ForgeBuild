#include "forge/compiler_identity.hpp"

#include <cassert>
#include <iostream>
#include <string>


int main()
{
    forge::CompilerIdentityCache cache;


    std::string identity1;
    std::string identity2;


    const bool result1 =
        cache.get(
            "c++ -std=c++20 -c input.cpp -o input.o",
            identity1
        );


    const bool result2 =
        cache.get(
            "c++ -O2 -c another.cpp -o another.o",
            identity2
        );


    std::cout
        << std::boolalpha
        << "compiler identity result1: "
        << result1
        << '\n';


    std::cout
        << "compiler identity result2: "
        << result2
        << '\n';


    std::cout
        << "compiler identity1: "
        << identity1
        << '\n';


    std::cout
        << "compiler identity2: "
        << identity2
        << '\n';


    assert(result1);
    assert(result2);

    //
    // Command 参数不同，但 compiler 都是 c++。
    //
    // Compiler Identity 应完全相同。
    //
    assert(
        identity1
        == identity2
    );


    std::string missing_identity;


    const bool missing_result =
        cache.get(
            "forge_compiler_that_does_not_exist -c input.cpp",
            missing_identity
        );


    std::cout
        << "missing compiler result: "
        << missing_result
        << '\n';


    assert(
        !missing_result
    );


    std::cout
        << "compiler identity checks passed\n";


    return 0;
}