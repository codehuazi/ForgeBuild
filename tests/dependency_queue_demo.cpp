#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>


int main()
{
    std::mutex mutex;

    std::mutex output_mutex;

    std::condition_variable condition;

    std::queue<std::string> tasks;


    //
    // 还有多少个编译任务尚未完成。
    //
    int remaining_compiles = 3;


    //
    // link 是否已经执行完成。
    //
    bool link_finished = false;


    //
    // 是否要求所有 Worker 停止。
    //
    bool stopping = false;


    auto work =
        [&](int worker_id)
        {
            {
                std::lock_guard<std::mutex> output_lock(
                    output_mutex
                );


                std::cout
                    << "worker "
                    << worker_id
                    << ": started\n";
            }


            while(true)
            {
                std::string task;


                //
                // 第一段临界区：
                // 等待并从共享队列取出任务。
                //
                {
                    std::unique_lock<std::mutex> lock(
                        mutex
                    );


                    condition.wait(
                        lock,
                        [&]()
                        {
                            return stopping
                                || !tasks.empty();
                        }
                    );


                    if(stopping
                        && tasks.empty())
                    {
                        break;
                    }


                    task =
                        tasks.front();


                    tasks.pop();
                }


                {
                    std::lock_guard<std::mutex> output_lock(
                        output_mutex
                    );


                    std::cout
                        << "worker "
                        << worker_id
                        << ": executing "
                        << task
                        << "\n";
                }


                std::this_thread::sleep_for(
                    std::chrono::seconds(2)
                );


                {
                    std::lock_guard<std::mutex> output_lock(
                        output_mutex
                    );


                    std::cout
                        << "worker "
                        << worker_id
                        << ": finished "
                        << task
                        << "\n";
                }


                //
                // 第二段临界区：
                // 提交任务完成后的调度状态。
                //
                {
                    std::lock_guard<std::mutex> lock(
                        mutex
                    );


                    if(task != "link app")
                    {
                        remaining_compiles--;


                        if(remaining_compiles == 0)
                        {
                            tasks.push(
                                "link app"
                            );
                        }
                    }
                    else
                    {
                        link_finished =
                            true;

                        stopping =
                            true;
                    }
                }


                //
                // 可能产生了新的 link 任务，
                // 或者 link 完成后要求所有 Worker 退出。
                //
                condition.notify_all();
            }


            {
                std::lock_guard<std::mutex> output_lock(
                    output_mutex
                );


                std::cout
                    << "worker "
                    << worker_id
                    << ": stopped\n";
            }
        };


    std::vector<std::thread> workers;


    workers.emplace_back(
        work,
        1
    );


    workers.emplace_back(
        work,
        2
    );


    //
    // 初始只加入没有前置依赖的三个编译任务。
    // link app 此时不能进入队列。
    //
    {
        std::lock_guard<std::mutex> lock(
            mutex
        );


        tasks.push(
            "compile a.cpp"
        );


        tasks.push(
            "compile b.cpp"
        );


        tasks.push(
            "compile main.cpp"
        );
    }


    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex
        );


        std::cout
            << "main: compile tasks prepared\n";
    }


    condition.notify_all();


    for(auto& worker :
        workers)
    {
        worker.join();
    }


    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex
        );


        std::cout
            << "main: link finished = "
            << std::boolalpha
            << link_finished
            << "\n";
    }


    return 0;
}