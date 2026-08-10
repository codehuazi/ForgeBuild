#include "forge/depfile.hpp"

#include <iostream>


int main()
{
    const std::string depfile_text =
        "main.o: main.cpp \\\n"
        " config.h \\\n"
        " math.h\n";


    forge::Depfile depfile;


    if (!depfile.parse(
            depfile_text
        ))
    {
        std::cerr
            << "failed to parse depfile\n";

        return 1;
    }


    std::cout
        << "output:\n"
        << depfile.output()
        << "\n\n";


    std::cout
        << "inputs:\n";


    for (const std::string& input :
         depfile.inputs())
    {
        std::cout
            << input
            << '\n';
    }


    return 0;
}