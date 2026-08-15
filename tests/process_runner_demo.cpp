#include "forge/process_runner.hpp"

#include <cerrno>
#include <csignal>
#include <iostream>


int main()
{
    forge::ProcessRunner runner;


    /*
     * Case 1：
     * 正常成功退出。
     */
    const forge::ProcessResult success =
        runner.run(
            "exit 0"
        );


    if(!success.spawned
        || !success.exited
        || success.exit_code != 0
        || success.signaled
        || !success.success())
    {
        std::cerr
            << "successful process result is incorrect\n";

        return 1;
    }


    /*
     * Case 2：
     * 正常退出，但退出码非 0。
     */
    const forge::ProcessResult failure =
        runner.run(
            "exit 7"
        );


    if(!failure.spawned
        || !failure.exited
        || failure.exit_code != 7
        || failure.signaled
        || failure.success())
    {
        std::cerr
            << "non-zero exit result is incorrect\n";

        return 1;
    }


    /*
     * Case 3：
     * 当前 Shell 自己被 SIGTERM 终止。
     *
     * $$ 表示当前 /bin/sh 的 PID。
     */
    const forge::ProcessResult signaled =
        runner.run(
            "kill -TERM $$"
        );


    if(!signaled.spawned
        || signaled.exited
        || !signaled.signaled
        || signaled.signal_number != SIGTERM
        || signaled.success())
    {
        std::cerr
            << "signal termination result is incorrect\n";

        return 1;
    }


    /*
     * Case 4：
     * 使用一个确定不存在的 Shell，
     * 验证 posix_spawn 创建失败。
     */
    forge::ProcessRunner missing_shell(
        "/definitely/not/a/real/forgebuild-shell"
    );


    const forge::ProcessResult spawn_failure =
        missing_shell.run(
            "exit 0"
        );


    if(spawn_failure.spawned
        || spawn_failure.error_number != ENOENT
        || spawn_failure.success())
    {
        std::cerr
            << "spawn failure result is incorrect\n";

        return 1;
    }


    std::cout
        << "process runner checks passed\n";


    return 0;
}