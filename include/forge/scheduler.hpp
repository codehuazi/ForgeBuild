#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>
#include <atomic>


namespace forge
{

class Edge;
class Executor;


class Scheduler
{

public:

    explicit Scheduler(
        int jobs
    );


    bool run(
        const std::vector<Edge*>& edges
    );


    void set_executor(
        Executor* executor
    );


private:

    void worker();

    void finish_edge(
        Edge* edge
    );


    int jobs_;

    Executor* executor_{nullptr};


    std::unordered_map<
        Edge*,
        int
    > indegree_;


    std::queue<
        Edge*
    > ready_queue_;


        //
    // 保护调度器共享状态：
    // indegree_、ready_queue_、计数器和状态标志。
    //
    std::mutex mutex_;


    //
    // 只保护多线程日志输出，
    // 避免一整行日志被其他线程的输出打断。
    //
    std::mutex output_mutex_;


    std::condition_variable condition_;

    std::condition_variable finished_condition_;


    //
    // 本次 run() 需要执行的 Edge 总数。
    //
    std::size_t total_edges_{0};


    //
    // 已经成功执行完成的 Edge 数量。
    //
    std::size_t completed_edges_{0};


    //
    // 已经从 Ready Queue 取出，
    // 但尚未执行完成的 Edge 数量。
    //
    std::size_t running_edges_{0};


    //
    // true 表示 Worker 应准备退出。
    //
    bool stopping_{false};


    //
    // true 表示至少有一个 Edge 执行失败。
    //
    std::atomic<bool> failed_{false};


};

}