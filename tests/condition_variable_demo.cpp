#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>


int main()
{
    std::mutex mutex;

    std::condition_variable condition;

    bool task_ready = false;

    std::string task;


    std::thread worker(
        [&]()
        {
            std::cout
                << "worker: started\n";


            std::unique_lock<std::mutex> lock(
                mutex
            );


            condition.wait(
                lock,
                [&]()
                {
                    return task_ready;
                }
            );


            std::cout
                << "worker: received task: "
                << task
                << "\n";
        }
    );


    std::this_thread::sleep_for(
        std::chrono::seconds(2)
    );


    {
        std::lock_guard<std::mutex> lock(
            mutex
        );


        task =
            "compile a.cpp";


        task_ready =
            true;


        std::cout
            << "main: task prepared\n";
    }


    condition.notify_one();


    worker.join();


    std::cout
        << "main: worker finished\n";


    return 0;
}