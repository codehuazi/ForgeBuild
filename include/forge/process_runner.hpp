#pragma once

#include <string>


namespace forge
{


struct ProcessResult
{
    bool spawned{
        false
    };


    bool exited{
        false
    };


    int exit_code{
        -1
    };


    bool signaled{
        false
    };


    int signal_number{
        0
    };


    int error_number{
        0
    };


    bool success() const noexcept
    {
        return spawned
            && exited
            && exit_code == 0;
    }
};


class ProcessRunner
{
public:

    ProcessRunner();


    explicit ProcessRunner(
        std::string shell_path
    );


    ProcessResult run(
        const std::string& command
    ) const;


private:

    std::string shell_path_;
};


}