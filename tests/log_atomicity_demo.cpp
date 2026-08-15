#include "forge/build_log.hpp"
#include "forge/deps_log.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


int main()
{
    const std::string root =
        ".forge_log_atomicity_test";

    const std::string build_log_path =
        root + "/build.log";

    const std::string deps_log_path =
        root + "/deps.log";


    std::filesystem::remove_all(
        root
    );

    std::filesystem::create_directory(
        root
    );


    forge::BuildLog build_log;

    build_log.record(
        "a.o",
        111
    );


    forge::DepsLog deps_log;

    deps_log.record(
        "a.o",
        {
            "a.cpp",
            "old.hpp"
        }
    );


    if(!build_log.save(
            build_log_path
        )
        || !deps_log.save(
            deps_log_path
        ))
    {
        std::cerr
            << "failed to save initial logs\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    //
    // 修改内存中的下一代状态。
    //
    build_log.record(
        "a.o",
        222
    );

    deps_log.record(
        "a.o",
        {
            "a.cpp",
            "new.hpp"
        }
    );


    //
    // 移除目录写权限。
    //
    // 已存在的最终文件仍可被直接打开，
    // 但不能在目录中创建新的临时文件。
    //
    // 因此旧的“直接覆盖 final file”实现会错误地
    // 成功；新的 atomic save 必须失败，而且不能
    // 破坏上一份可靠日志。
    //
    std::error_code permission_error;


    std::filesystem::permissions(
        root,
        std::filesystem::perms::owner_read
            | std::filesystem::perms::owner_exec,
        std::filesystem::perm_options::replace,
        permission_error
    );


    if(permission_error)
    {
        std::cerr
            << "failed to change directory permissions\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    const bool build_save_result =
        build_log.save(
            build_log_path
        );

    const bool deps_save_result =
        deps_log.save(
            deps_log_path
        );


    //
    // 无论测试结果如何，先恢复权限，
    // 保证后续读取和 cleanup 能正常执行。
    //
    std::filesystem::permissions(
        root,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace,
        permission_error
    );


    if(permission_error)
    {
        std::cerr
            << "failed to restore directory permissions\n";

        return 1;
    }


    if(build_save_result
        || deps_save_result)
    {
        std::cerr
            << "atomic save unexpectedly succeeded "
            << "in read-only directory\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    //
    // 保存失败后，磁盘上的旧一代状态必须完整保留。
    //
    forge::BuildLog loaded_build_log;

    forge::DepsLog loaded_deps_log;


    if(!loaded_build_log.load(
            build_log_path
        )
        || !loaded_deps_log.load(
            deps_log_path
        ))
    {
        std::cerr
            << "failed to reload preserved logs\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    if(loaded_build_log.command_hash(
            "a.o"
        ) != 111)
    {
        std::cerr
            << "failed save corrupted previous build log\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    const std::vector<std::string>
        expected_old_dependencies{
            "a.cpp",
            "old.hpp"
        };


    if(loaded_deps_log.inputs(
            "a.o"
        ) != expected_old_dependencies)
    {
        std::cerr
            << "failed save corrupted previous deps log\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    //
    // 权限恢复以后，新一代状态应该可以正常提交。
    //
    if(!build_log.save(
            build_log_path
        )
        || !deps_log.save(
            deps_log_path
        ))
    {
        std::cerr
            << "failed to save logs after restoring permissions\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    forge::BuildLog final_build_log;

    forge::DepsLog final_deps_log;


    if(!final_build_log.load(
            build_log_path
        )
        || !final_deps_log.load(
            deps_log_path
        ))
    {
        std::cerr
            << "failed to load final logs\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    if(final_build_log.command_hash(
            "a.o"
        ) != 222)
    {
        std::cerr
            << "final build log contains stale state\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    const std::vector<std::string>
        expected_new_dependencies{
            "a.cpp",
            "new.hpp"
        };


    if(final_deps_log.inputs(
            "a.o"
        ) != expected_new_dependencies)
    {
        std::cerr
            << "final deps log contains stale state\n";

        std::filesystem::remove_all(
            root
        );

        return 1;
    }


    //
    // 成功提交后不能残留临时文件。
    //
    for(const auto& entry :
        std::filesystem::directory_iterator(
            root
        ))
    {
        if(entry.path()
                .filename()
                .string()
                .find(".tmp.")
            != std::string::npos)
        {
            std::cerr
                << "temporary log file was not cleaned up\n";

            std::filesystem::remove_all(
                root
            );

            return 1;
        }
    }


    std::filesystem::remove_all(
        root
    );


    std::cout
        << "log atomicity checks passed\n";


    return 0;
}