#pragma once

#include "task_queue.h"
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>

class thread_pool {
public:
    thread_pool(
        int num_threads);
    ~thread_pool();

    void start();
    void stop();
    void pause();
    void resume();
    bool add_task(int thread_id);
    void print_average_wait_times();

private:
    void worker_loop(int thread_id);

    task_queue m_task_queue;
    std::condition_variable m_not_empty_cv;
    std::condition_variable m_not_full_cv;
    std::mutex m_cv_mutex;

    std::vector<std::thread> m_threads;
    std::atomic<bool> m_stop;
    std::atomic<bool> m_paused;
    int m_num_threads;
    std::atomic<int> m_task_id_counter;
    std::vector<std::vector<double>> m_wait_times;
    std::mutex m_wait_times_mutex;
};
