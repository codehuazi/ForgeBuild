#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>


int main()
{
    std::mutex mutex;

    std::condition_variable condition;

    std::queue<std::string> tasks;

    bool stopping = false;


    std::thread worker(
        [&]()
        {
            std::cout
                << "worker: started\n";


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


                std::cout
                    << "worker: executing "
                    << task
                    << "\n";


                std::this_thread::sleep_for(
                    std::chrono::seconds(1)
                );


                std::cout
                    << "worker: finished "
                    << task
                    << "\n";
            }


            std::cout
                << "worker: stopped\n";
        }
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
            "link app"
        );


        std::cout
            << "main: three tasks prepared\n";
    }


    condition.notify_one();


    std::this_thread::sleep_for(
        std::chrono::seconds(4)
    );


    {
        std::lock_guard<std::mutex> lock(
            mutex
        );


        stopping =
            true;
    }


    condition.notify_one();


    worker.join();


    std::cout
        << "main: worker finished\n";


    return 0;
}