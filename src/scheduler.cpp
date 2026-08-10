#include "forge/scheduler.hpp"

#include "forge/edge.hpp"
#include "forge/node.hpp"
#include "forge/executor.hpp"

#include <iostream>
#include <thread>
#include <unordered_set>


namespace forge
{

void Scheduler::finish_edge(
    Edge* edge
)
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    completed_edges_++;

    running_edges_--;


    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex_
        );


        std::cout
            << "finished: "
            << edge->describe()
            << "\n";
    }


    /*
    * 同一个 dependent 可能同时使用当前 Edge
    * 生成的多个输出文件。
    *
    * 当前 Edge 完成时，对这个 dependent
    * 只能释放一次任务依赖。
    */
    std::unordered_set<Edge*> unique_dependents;


    for(auto* output :
        edge->outputs())
    {
        for(auto* dependent :
            output->out_edges())
        {
            if(!indegree_.contains(
                dependent
            ))
            {
                continue;
            }


            unique_dependents.insert(
                dependent
            );
        }
    }


    for(auto* dependent :
        unique_dependents)
    {
        indegree_[dependent]--;


        if(indegree_[dependent] == 0)
        {
            ready_queue_.push(
                dependent
            );
        }
    }


    finished_condition_.notify_all();

}


Scheduler::Scheduler(
    int jobs
)
    :
    jobs_(jobs)
{

}

void Scheduler::set_executor(
    Executor* executor
)
{
    executor_ = executor;
}

void Scheduler::worker()
{

    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex_
        );


        std::cout
            << "worker started\n";
    }


    while(true)
    {
        Edge* edge =
            nullptr;


        {
            std::unique_lock<std::mutex> lock(
                mutex_
            );


            condition_.wait(
                lock,
                [this]()
                {
                    return stopping_
                        || failed_
                        || !ready_queue_.empty();
                }
            );


            if(stopping_
                || failed_)
            {
                break;
            }


            edge =
                ready_queue_.front();


            ready_queue_.pop();


            running_edges_++;
        }


        {
            std::lock_guard<std::mutex> output_lock(
                output_mutex_
            );


            std::cout
                << "worker picked: "
                << edge->describe()
                << "\n";
        }

        const bool success =
            executor_->execute_edge(
                edge
            );



        if(!success)
        {
            {
                std::lock_guard<std::mutex> lock(
                    mutex_
                );


                failed_ = true;


                stopping_ = true;
            }


            /*
            * 唤醒正在等待任务的其他 Worker，
            * 让它们看到 stopping_ 后退出。
            */
            condition_.notify_all();


            /*
            * 唤醒 run() 中等待构建结束的主线程。
            *
            * 主线程的等待条件包含 failed_，
            * 但状态变化本身不会自动唤醒条件变量。
            */
            finished_condition_.notify_all();


            return;
        }


        finish_edge(
            edge
        );



        condition_.notify_all();
    }


    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex_
        );


        std::cout
            << "worker stopped\n";
    }

}

bool Scheduler::run(
    const std::vector<Edge*>& edges
)
{

    if (jobs_ <= 0)
    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex_
        );


        std::cerr
            << "scheduler requires jobs > 0\n";


        return false;
    }


    if (executor_ == nullptr)
    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex_
        );


        std::cerr
            << "scheduler executor is not set\n";


        return false;
    }

    //
    // 每次 run() 都代表一次新的调度过程，
    // 不能继承上一次运行留下的状态。
    //
    total_edges_ =
        edges.size();


    completed_edges_ =
        0;


    running_edges_ =
        0;


    stopping_ =
        false;


    failed_ =
        false;



    //
    // Scheduler 对象可能重复调用 run()，
    // 因此每次运行前清空上一次的状态。
    //
    indegree_.clear();


    ready_queue_ =
        std::queue<Edge*>{};



    //
    // 第一步：
    // 初始化所有 Edge 的入度。
    //
    for(auto* edge : edges)
    {
        indegree_[edge] = 0;
    }



    //
    // 第二步：
    // 根据输入 Node 的生产者 Edge，
    // 计算当前 Edge 的前置依赖数量。
    //
    for(auto* edge : edges)
    {
        /*
        * 一个 Edge 可能同时使用同一个 producer
        * 生成的多个输出文件。
        *
        * Scheduler 计算的是任务依赖数量，
        * 因此同一个 producer 只能计入一次。
        */
        std::unordered_set<Edge*> unique_producers;


        for(auto* input :
            edge->inputs())
        {
            Edge* producer =
                input->in_edge();


            if(producer == nullptr)
            {
                continue;
            }


            /*
            * 只计算当前调度计划中的生产者。
            *
            * 计划外的 producer 表示其输出已经存在，
            * 本次不需要重新执行。
            */
            if(!indegree_.contains(
                producer
            ))
            {
                continue;
            }


            unique_producers.insert(
                producer
            );
        }


        indegree_[edge] =
            static_cast<int>(
                unique_producers.size()
            );
    }



    //
    // 第三步：
    // 将所有入度为零的 Edge 放入 Ready Queue。
    //
    for(auto& [edge, degree] :
        indegree_)
    {

        if(degree == 0)
        {
            ready_queue_.push(
                edge
            );
        }

    }


    //
    // 创建 Worker。
    //
    // 当前阶段 Worker 会领取初始 Ready Edge，
    // 但暂时不会执行命令或释放下游依赖。
    //
    std::vector<std::thread> workers;


    for(int i = 0; i < jobs_; ++i)
    {
        workers.emplace_back(
            &Scheduler::worker,
            this
        );
    }


    condition_.notify_all();


    {
        std::unique_lock<std::mutex> lock(
            mutex_
        );


        finished_condition_.wait(
            lock,
            [this]()
            {
                return failed_
                    || completed_edges_
                        == total_edges_;
            }
        );
    }


    // 初始 Ready Queue 已被领取完毕，
    // 通知所有 Worker 退出。
    //
    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        stopping_ =
            true;
    }


    condition_.notify_all();


    for(auto& thread :
        workers)
    {
        thread.join();
    }


    return !failed_;


}


}