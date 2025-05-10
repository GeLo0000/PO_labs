#include "task_queue.h"
#include "thread_pool.h"
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <iostream>
#include <ctime>

std::atomic<bool> stop_generators = false;

void start_generators(thread_pool& worker, int num_generator, int time) {
    while (!stop_generators.load()) {
        worker.add_task(num_generator, stop_generators);
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }
}


int main() {
    const int num_generators = 2;
    const int num_workers = 6;

    std::vector<std::thread> generator_threads;

    thread_pool worker(num_workers);
    worker.start();

    for (int i = 0; i < num_generators; ++i) {
        int time = rand() % 1000;
        generator_threads.emplace_back(start_generators, std::ref(worker), i, time);
    }

    std::this_thread::sleep_for(std::chrono::seconds(15));

    std::cout << "\n[Main] Pausing consumers...\n";
    worker.pause();

    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\n[Main] Resuming consumers...\n";
    worker.resume();

    std::this_thread::sleep_for(std::chrono::seconds(15));

    
    std::cout << "\n[Main] Stopping all...\n";
    stop_generators = true;
    worker.Notify_all_cv_not_full();

    for (auto& t : generator_threads) {
        if (t.joinable())
            t.join();
    }

    worker.stop();
    std::cout << "\n[Main] Finish all...\n";

    return 0;
}

