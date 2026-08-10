#include "forge/file_system.hpp"
#include "forge/node.hpp"

#include <cassert>
#include <iostream>
#include <string>


int main()
{
    const std::string source_path =
        "main.cpp";

    const std::string output_path =
        "main.o";

    const std::string missing_path =
        "missing.file";


    // 第一部分：直接测试 FileSystem。
    bool source_exists =
        forge::FileSystem::exists(
            source_path
        );

    bool output_exists =
        forge::FileSystem::exists(
            output_path
        );

    bool missing_exists =
        forge::FileSystem::exists(
            missing_path
        );


    std::cout
        << std::boolalpha;

    std::cout
        << "main.cpp exists: "
        << source_exists
        << '\n';

    std::cout
        << "main.o exists: "
        << output_exists
        << '\n';

    std::cout
        << "missing.file exists: "
        << missing_exists
        << '\n';


    assert(source_exists);

    assert(output_exists);

    assert(!missing_exists);


    // 注意：时间戳变量必须先定义，
    // 后面的 Node 测试才能使用它们。
    long long source_time =
        forge::FileSystem::timestamp(
            source_path
        );

    long long output_time =
        forge::FileSystem::timestamp(
            output_path
        );

    long long missing_time =
        forge::FileSystem::timestamp(
            missing_path
        );


    std::cout
        << "main.cpp timestamp: "
        << source_time
        << '\n';

    std::cout
        << "main.o timestamp: "
        << output_time
        << '\n';

    std::cout
        << "missing.file timestamp: "
        << missing_time
        << '\n';


    assert(source_time != 0);

    assert(output_time != 0);

    assert(missing_time == 0);


    // 第二部分：测试 Node::refresh()。
    forge::Node source_node(
        source_path
    );

    forge::Node output_node(
        output_path
    );

    forge::Node missing_node(
        missing_path
    );


    source_node.refresh();

    output_node.refresh();

    missing_node.refresh();


    std::cout
        << "source node exists: "
        << source_node.exists()
        << '\n';

    std::cout
        << "source node timestamp: "
        << source_node.timestamp()
        << '\n';

    std::cout
        << "output node exists: "
        << output_node.exists()
        << '\n';

    std::cout
        << "output node timestamp: "
        << output_node.timestamp()
        << '\n';

    std::cout
        << "missing node exists: "
        << missing_node.exists()
        << '\n';

    std::cout
        << "missing node timestamp: "
        << missing_node.timestamp()
        << '\n';


    assert(source_node.exists());

    assert(output_node.exists());

    assert(!missing_node.exists());


    assert(
        source_node.timestamp()
        ==
        source_time
    );

    assert(
        output_node.timestamp()
        ==
        output_time
    );

    assert(
        missing_node.timestamp()
        ==
        missing_time
    );


    std::cout
        << "file system checks passed\n";

    return 0;
}