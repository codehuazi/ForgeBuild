#include "forge/process_runner.hpp"

#include <cerrno>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utility>


extern char** environ;


namespace forge
{


ProcessRunner::ProcessRunner()
    :
    shell_path_(
        "/bin/sh"
    )
{
}


ProcessRunner::ProcessRunner(
    std::string shell_path
)
    :
    shell_path_(
        std::move(
            shell_path
        )
    )
{
}


ProcessResult ProcessRunner::run(
    const std::string& command
) const
{
    ProcessResult result;


    /*
     * 使用 /bin/sh -c 保留现有 Manifest 的 Shell 语义，
     * 例如：
     *
     *   &&
     *   >
     *   |
     *
     * ProcessRunner 只负责进程创建与状态回收，
     * 不负责自己解析 Shell 命令。
     */
    std::string shell =
        shell_path_;


    std::string option =
        "-c";


    std::string command_copy =
        command;


    char* arguments[] =
    {
        shell.data(),
        option.data(),
        command_copy.data(),
        nullptr
    };


    pid_t process_id =
        -1;


    const int spawn_error =
        ::posix_spawn(
            &process_id,
            shell_path_.c_str(),
            nullptr,
            nullptr,
            arguments,
            environ
        );


    if(spawn_error != 0)
    {
        /*
         * posix_spawn() 直接返回错误号，
         * 不能像普通系统调用一样只读取 errno。
         */
        result.error_number =
            spawn_error;


        return result;
    }


    result.spawned =
        true;


    int status =
        0;


    pid_t wait_result =
        -1;


    do
    {
        wait_result =
            ::waitpid(
                process_id,
                &status,
                0
            );
    }
    while(wait_result == -1
        && errno == EINTR);


    if(wait_result == -1)
    {
        result.error_number =
            errno;


        return result;
    }


    if(WIFEXITED(
            status
        ))
    {
        result.exited =
            true;


        result.exit_code =
            WEXITSTATUS(
                status
            );


        return result;
    }


    if(WIFSIGNALED(
            status
        ))
    {
        result.signaled =
            true;


        result.signal_number =
            WTERMSIG(
                status
            );


        return result;
    }


    return result;
}


}