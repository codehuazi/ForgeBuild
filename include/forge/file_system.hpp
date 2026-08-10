#pragma once

#include <string>


namespace forge
{


class FileSystem
{

public:

    static bool exists(
        const std::string& path
    );


    static long long timestamp(
        const std::string& path
    );

};


}