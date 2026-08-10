#include "forge/local_cache.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>


int main()
{
    const std::string cache_root =
        ".forge_cache_test";


    std::filesystem::remove_all(
        cache_root
    );


    const std::string output =
        "cache_output.o";


    {
        std::ofstream file(
            output,
            std::ios::binary
        );


        file
            << "fake object data";
    }


    const std::uint64_t key =
        0xabc123;


    forge::LocalCache cache(
        cache_root
    );


    assert(
        !cache.contains(key)
    );


    assert(
        cache.store(
            key,
            output
        )
    );


    assert(
        cache.contains(key)
    );


    //
    // 原子 store 完成后不应该残留临时文件。
    //
    for(const auto& entry :
        std::filesystem::
            directory_iterator(
                cache_root
            ))
    {
        assert(
            entry.path()
                .filename()
                .string()
                .find(
                    ".tmp."
                )
            == std::string::npos
        );
    }


    std::filesystem::remove(
        output
    );


    assert(
        cache.restore(
            key,
            output
        )
    );


    std::ifstream restored(
        output,
        std::ios::binary
    );


    std::string content(
        (
            std::istreambuf_iterator<char>(
                restored
            )
        ),
        std::istreambuf_iterator<char>()
    );


    assert(
        content
        == "fake object data"
    );


    restored.close();


    //
    // 人为破坏 cache object。
    //
    const std::string cached_object =
        cache_root
        + "/abc123";


    {
        std::ofstream corrupted(
            cached_object,
            std::ios::binary
            | std::ios::trunc
        );


        corrupted
            << "corrupted";
    }


    std::filesystem::remove(
        output
    );


    //
    // restore 必须识别损坏，
    // 返回 false，而不是恢复错误数据。
    //
    assert(
        !cache.restore(
            key,
            output
        )
    );


    assert(
        !std::filesystem::exists(
            output
        )
    );


    //
    // 损坏对象应该被清除。
    //
    assert(
        !cache.contains(key)
    );


    std::filesystem::remove_all(
        cache_root
    );


    std::filesystem::remove(
        output
    );


    std::cout
        << "local cache atomic/corruption checks passed\n";


    return 0;
}