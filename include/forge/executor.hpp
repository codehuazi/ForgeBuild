#pragma once

#include "forge/compiler_identity.hpp"

#include <mutex>

namespace forge
{

class BuildLog;
class BuildPlan;
class Edge;
class DepsLog;
class LocalCache;


class Executor
{

public:

    Executor();


    explicit Executor(
        BuildLog& build_log
    );


    Executor(
        BuildLog& build_log,
        DepsLog& deps_log
    );


    Executor(
        BuildLog& build_log,
        DepsLog& deps_log,
        LocalCache& local_cache
    );


    //
    // 执行一个完整的构建计划。
    // 该接口保留给串行执行场景使用。
    //
    bool execute(
        const BuildPlan& plan
    );


    //
    // 执行单个 Edge。
    // 这是 Scheduler 后续分发给 Worker 的最小任务单位。
    //
    bool execute_edge(
        Edge* edge
    );


private:

    //
    // Executor 不拥有 BuildLog。
    // nullptr 表示当前执行器不记录构建日志。
    //
    BuildLog* build_log_;

    //
    // 保护 Executor 内部日志输出。
    //
    std::mutex output_mutex_;

    DepsLog* deps_log_;

    LocalCache* local_cache_;

    //
    // Executor 会被多个 Scheduler Worker 共享。
    // CompilerIdentityCache 内部负责线程安全，
    // 避免对同一编译器重复解析路径和计算文件 Hash。
    //
    CompilerIdentityCache compiler_identity_cache_;
};

}