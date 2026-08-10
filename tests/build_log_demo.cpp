#include "forge/build_log.hpp"
#include "forge/hash.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>


int main()
{
    forge::BuildLog build_log;


    const std::string output =
        "main.o";

    const std::string old_command =
        "g++ -c main.cpp -o main.o";

    const std::string new_command =
        "g++ -O2 -c main.cpp -o main.o";


    const std::uint64_t old_hash =
        forge::hash_string(old_command);

    const std::uint64_t new_hash =
        forge::hash_string(new_command);


    // 初始状态没有任何记录。
    assert(!build_log.contains(output));

    assert(
        build_log.command_hash(output)
        == 0
    );

    assert(
        !build_log.command_matches(
            output,
            old_hash
        )
    );


    // 保存第一次构建使用的命令哈希。
    build_log.record(
        output,
        old_hash
    );

    assert(build_log.contains(output));

    assert(
        build_log.command_hash(output)
        == old_hash
    );

    assert(
        build_log.command_matches(
            output,
            old_hash
        )
    );

    assert(
        !build_log.command_matches(
            output,
            new_hash
        )
    );


    // 用新命令覆盖旧记录。
    build_log.record(
        output,
        new_hash
    );

    assert(
        build_log.command_hash(output)
        == new_hash
    );

    assert(
        build_log.command_matches(
            output,
            new_hash
        )
    );

    assert(
        !build_log.command_matches(
            output,
            old_hash
        )
    );

        const std::string log_path =
        "build_log_demo.log";


    // 将当前记录保存到磁盘。
    assert(
        build_log.save(log_path)
    );


    // 新建另一个空日志对象，
    // 模拟程序重新启动。
    forge::BuildLog loaded_log;

    assert(
        !loaded_log.contains(output)
    );


    // 从磁盘恢复记录。
    assert(
        loaded_log.load(log_path)
    );

    assert(
        loaded_log.contains(output)
    );

    assert(
        loaded_log.command_hash(output)
        == new_hash
    );

    assert(
        loaded_log.command_matches(
            output,
            new_hash
        )
    );

    assert(
        !loaded_log.command_matches(
            output,
            old_hash
        )
    );

    std::filesystem::remove(
        log_path
    );

    std::cout
        << "old hash: "
        << old_hash
        << '\n';

    std::cout
        << "new hash: "
        << new_hash
        << '\n';

    std::cout
        << "build log memory and disk checks passed\n";


    return 0;
}