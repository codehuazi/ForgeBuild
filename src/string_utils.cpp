#include "forge/string_utils.hpp"
#include <sstream>
#include <vector>


namespace forge {


std::string trim(
    const std::string& text
)
{
    size_t begin = 0;


    while(begin < text.size()
          && std::isspace(text[begin]))
    {
        begin++;
    }


    size_t end = text.size();


    while(end > begin
          && std::isspace(text[end - 1]))
    {
        end--;
    }


    return text.substr(
        begin,
        end - begin
    );
}



bool starts_with(
    const std::string& text,
    const std::string& prefix
)
{
    return text.rfind(prefix,0)==0;
}

std::vector<std::string> split(
    const std::string& str,
    char delimiter
)
{
    std::vector<std::string> result;


    std::stringstream ss(str);


    std::string item;


    while(
        std::getline(
            ss,
            item,
            delimiter
        )
    )
    {
        if(!item.empty())
        {
            result.push_back(
                item
            );
        }
    }


    return result;
}

std::vector<std::string> split(
    const std::string& str
)
{
    std::vector<std::string> result;


    std::stringstream ss(str);


    std::string item;


    while(ss >> item)
    {
        result.push_back(item);
    }


    return result;
}

void replace_all(
    std::string& str,
    const std::string& from,
    const std::string& to
)
{

    if(from.empty())
    {
        return;
    }


    size_t pos = 0;


    while(
        (pos = str.find(from, pos))
        != std::string::npos
    )
    {

        str.replace(
            pos,
            from.length(),
            to
        );


        pos += to.length();

    }

}

}