#include "forge/hash.hpp"
#include "forge/cache_key.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>


int main()
{
    const std::string command1 =
        "g++ -c main.cpp -o main.o";

    const std::string command2 =
        "g++ -c main.cpp -o main.o";

    const std::string command3 =
        "g++ -O2 -c main.cpp -o main.o";


    const std::uint64_t hash1 =
        forge::hash_string(command1);

    const std::uint64_t hash2 =
        forge::hash_string(command2);

    const std::uint64_t hash3 =
        forge::hash_string(command3);


    std::cout
        << "command1 hash: "
        << hash1
        << '\n';

    std::cout
        << "command2 hash: "
        << hash2
        << '\n';

    std::cout
        << "command3 hash: "
        << hash3
        << '\n';


    assert(hash1 == hash2);

    assert(hash1 != hash3);

    forge::Hash64 incremental_hasher;


    incremental_hasher.update(
        "g++ -c "
    );


    incremental_hasher.update(
        "main.cpp "
    );


    incremental_hasher.update(
        "-o main.o"
    );


    const std::uint64_t incremental_hash =
        incremental_hasher.value();


    std::cout
        << "incremental hash: "
        << incremental_hash
        << '\n';


    assert(
        incremental_hash
        == hash1
    );

    {
        std::ofstream output(
            "hash_test_input.txt",
            std::ios::binary
        );


        output
            << "hello "
            << "ForgeBuild";
    }


    const std::uint64_t expected_file_hash =
        forge::hash_string(
            "hello ForgeBuild"
        );


    forge::Hash64 file_hasher;


    const bool file_hash_ok =
        file_hasher.update_file(
            "hash_test_input.txt"
        );


    const std::uint64_t file_hash =
        file_hasher.value();


    std::cout
        << "expected file hash: "
        << expected_file_hash
        << '\n';


    std::cout
        << "actual file hash: "
        << file_hash
        << '\n';


    assert(
        file_hash_ok
    );


    assert(
        file_hash
        == expected_file_hash
    );


    std::remove(
        "hash_missing_input.txt"
    );


    forge::Hash64 missing_file_hasher;


    missing_file_hasher.update(
        "prefix"
    );


    const std::uint64_t hash_before_failure =
        missing_file_hasher.value();


    const bool missing_file_ok =
        missing_file_hasher.update_file(
            "hash_missing_input.txt"
        );


    const std::uint64_t hash_after_failure =
        missing_file_hasher.value();


    std::cout
        << "missing file result: "
        << std::boolalpha
        << missing_file_ok
        << '\n';


    assert(
        !missing_file_ok
    );


    assert(
        hash_before_failure
        == hash_after_failure
    );

    forge::CacheKeyBuilder cache_key1;


    cache_key1.add_command(
        command1
    );


    forge::CacheKeyBuilder cache_key2;


    cache_key2.add_command(
        command2
    );


    forge::CacheKeyBuilder cache_key3;


    cache_key3.add_command(
        command3
    );


    const std::uint64_t key1 =
        cache_key1.value();


    const std::uint64_t key2 =
        cache_key2.value();


    const std::uint64_t key3 =
        cache_key3.value();


    std::cout
        << "cache key1: "
        << key1
        << '\n';


    std::cout
        << "cache key2: "
        << key2
        << '\n';


    std::cout
        << "cache key3: "
        << key3
        << '\n';


    assert(
        key1 == key2
    );


    assert(
        key1 != key3
    );

        const std::string compiler_identity1 =
        "/usr/bin/g++|gcc-11.4.0";

    const std::string compiler_identity2 =
        "/usr/bin/g++|gcc-11.4.0";

    const std::string compiler_identity3 =
        "/usr/bin/g++|gcc-13.2.0";


    forge::CacheKeyBuilder compiler_key1;

    compiler_key1.add_compiler_identity(
        compiler_identity1
    );

    compiler_key1.add_command(
        command1
    );


    forge::CacheKeyBuilder compiler_key2;

    compiler_key2.add_compiler_identity(
        compiler_identity2
    );

    compiler_key2.add_command(
        command1
    );


    forge::CacheKeyBuilder compiler_key3;

    compiler_key3.add_compiler_identity(
        compiler_identity3
    );

    compiler_key3.add_command(
        command1
    );


    const std::uint64_t compiler_cache_key1 =
        compiler_key1.value();

    const std::uint64_t compiler_cache_key2 =
        compiler_key2.value();

    const std::uint64_t compiler_cache_key3 =
        compiler_key3.value();


    std::cout
        << "compiler cache key1: "
        << compiler_cache_key1
        << '\n';

    std::cout
        << "compiler cache key2: "
        << compiler_cache_key2
        << '\n';

    std::cout
        << "compiler cache key3: "
        << compiler_cache_key3
        << '\n';


    assert(
        compiler_cache_key1
        == compiler_cache_key2
    );

    assert(
        compiler_cache_key1
        != compiler_cache_key3
    );

    {
        std::ofstream output(
            "cache_key_test.hpp",
            std::ios::binary
        );

        output
            << "constexpr int value = 10;\n";
    }


    forge::CacheKeyBuilder dependency_key1;

    dependency_key1.add_command(
        command1
    );

    const bool dependency1_ok =
        dependency_key1.add_dependency(
            "cache_key_test.hpp"
        );

    assert(
        dependency1_ok
    );

    const std::uint64_t dep_key1 =
        dependency_key1.value();


    {
        std::ofstream output(
            "cache_key_test.hpp",
            std::ios::binary
        );

        output
            << "constexpr int value = 20;\n";
    }


    forge::CacheKeyBuilder dependency_key2;

    dependency_key2.add_command(
        command1
    );

    const bool dependency2_ok =
        dependency_key2.add_dependency(
            "cache_key_test.hpp"
        );

    assert(
        dependency2_ok
    );

    const std::uint64_t dep_key2 =
        dependency_key2.value();


    std::cout
        << "dependency key1: "
        << dep_key1
        << '\n';

    std::cout
        << "dependency key2: "
        << dep_key2
        << '\n';


    assert(
        dep_key1 != dep_key2
    );


    std::cout
        << "hash checks passed\n";


    return 0;
}