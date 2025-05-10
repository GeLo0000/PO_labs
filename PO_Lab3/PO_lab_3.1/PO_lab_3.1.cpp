#include "task_queue.h"
#include "generator_tasks.h"
#include "thread_pool.h"
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <iostream>

std::condition_variable not_full_cv;
std::condition_variable not_empty_cv;
std::mutex cv_mutex_producer;
std::mutex cv_mutex_consumer;

int main() {
    task_queue queue;



    const int num_producers = 2;
    const int num_consumers = 6;

    generator_tasks generators(queue, not_full_cv, not_empty_cv, cv_mutex_producer, num_producers);
    thread_pool consumers(queue, not_full_cv, not_empty_cv, cv_mutex_consumer, num_consumers);

    generators.start();
    consumers.start();

    std::this_thread::sleep_for(std::chrono::seconds(15));

    std::cout << "\n[Main] Pausing consumers...\n";
    consumers.pause();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "\n[Main] Resuming consumers...\n";
    consumers.resume();

    std::this_thread::sleep_for(std::chrono::seconds(15));

    std::cout << "\n[Main] Stopping all...\n";
    generators.stop();
    consumers.stop();

    return 0;
}
