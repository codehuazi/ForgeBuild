#pragma once

#include <string>
#include <vector>


namespace forge
{

class Depfile
{

public:

    bool parse(
        const std::string& text
    );


    bool load(
        const std::string& file_path
    );


    const std::string& output() const;


    const std::vector<std::string>& inputs() const;


private:

    std::string output_;


    std::vector<std::string> inputs_;

};

}