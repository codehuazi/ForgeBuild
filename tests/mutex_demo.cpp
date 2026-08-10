#include <iostream>
#include <mutex>
#include <thread>
#include <vector>


int main()
{
    int counter = 0;

    std::mutex mutex;


    auto work =
        [&counter, &mutex]()
        {
            for(int i = 0; i < 1000000; ++i)
            {
                std::lock_guard<std::mutex> lock(
                    mutex
                );

                counter++;
            }
        };


    std::vector<std::thread> threads;


    for(int i = 0; i < 4; ++i)
    {
        threads.emplace_back(
            work
        );
    }


    for(auto& thread :
        threads)
    {
        thread.join();
    }


    std::cout
        << "counter = "
        << counter
        << "\n";


    return 0;
}