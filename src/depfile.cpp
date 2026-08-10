#include "forge/depfile.hpp"

#include <fstream>
#include <sstream>


namespace forge
{


bool Depfile::parse(
    const std::string& text
)
{
    output_.clear();
    inputs_.clear();


    /*
     * GCC/Clang 可能生成多行 depfile：
     *
     * main.o: main.cpp \
     *  config.h \
     *  math.h
     *
     * 其中反斜杠加换行表示当前行继续。
     *
     * 先将它们替换为空格，后面就可以把整个
     * 依赖列表当成一行处理。
     */
    std::string normalized;

    normalized.reserve(
        text.size()
    );


    for (std::size_t i = 0;
         i < text.size();
         ++i)
    {
        /*
         * Linux换行：
         *
         * \
         * \n
         */
        if (text[i] == '\\'
            && i + 1 < text.size()
            && text[i + 1] == '\n')
        {
            normalized.push_back(' ');

            ++i;

            continue;
        }


        /*
         * Windows换行：
         *
         * \
         * \r
         * \n
         */
        if (text[i] == '\\'
            && i + 2 < text.size()
            && text[i + 1] == '\r'
            && text[i + 2] == '\n')
        {
            normalized.push_back(' ');

            i += 2;

            continue;
        }


        normalized.push_back(
            text[i]
        );
    }


    const std::size_t colon =
        normalized.find(':');


    if (colon == std::string::npos)
    {
        return false;
    }


    output_ =
        normalized.substr(
            0,
            colon
        );


    std::string input_text =
        normalized.substr(
            colon + 1
        );


    std::stringstream stream(
        input_text
    );


    std::string item;


    while (stream >> item)
    {
        inputs_.push_back(
            item
        );
    }


    return true;
}


bool Depfile::load(
    const std::string& file_path
)
{

    std::ifstream input(
        file_path
    );


    if(!input)
    {
        return false;
    }


    std::stringstream buffer;


    buffer
        << input.rdbuf();


    return parse(
        buffer.str()
    );

}


const std::string& Depfile::output() const
{
    return output_;
}



const std::vector<std::string>& Depfile::inputs() const
{
    return inputs_;
}


}