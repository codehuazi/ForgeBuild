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


        tasks.push(
            "link app"
        );
    }


    {
        std::lock_guard<std::mutex> output_lock(
            output_mutex
        );


        std::cout
            << "main: four tasks prepared\n";
    }


    condition.notify_all();


    std::this_thread::sleep_for(
        std::chrono::seconds(5)
    );


    {
        std::lock_guard<std::mutex> lock(
            mutex
        );


        stopping =
            true;
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
            << "main: all workers finished\n";
    }


    return 0;
}