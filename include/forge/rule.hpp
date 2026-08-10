#pragma once

#include <string>


namespace forge
{


class Rule
{

public:

    Rule(
        std::string name,
        std::string command
    );


    const std::string& name() const;


    const std::string& command() const;


    void set_command(
        std::string command
    );


    const std::string& depfile() const;


    void set_depfile(
        std::string depfile
    );


private:

    std::string name_;

    std::string command_;

    /*
     * depfile路径模板。
     *
     * 例如：
     *
     *     $out.d
     *
     * 或者：
     *
     *     build/deps/main.d
     *
     * 空字符串表示该规则不生成depfile。
     */
    std::string depfile_;

};


}