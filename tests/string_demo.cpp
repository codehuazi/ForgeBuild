#include "forge/string_utils.hpp"

#include <cassert>
#include <iostream>


int main()
{

    auto result =
        forge::trim(
            "   hello forge   "
        );


    assert(
        result=="hello forge"
    );


    assert(
        forge::starts_with(
            "rule compile",
            "rule"
        )
    );


    assert(
        !forge::starts_with(
            "compile rule",
            "rule"
        )
    );

    auto parts =
    forge::split(
        "compile main.cpp",
        ' '
    );


    for(auto& p:parts)
    {
        std::cout
            << p
            << "\n";
    }


    std::cout
        <<"string utils passed\n";


    return 0;
}