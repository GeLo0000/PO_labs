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
    void add_task(int thread_id, std::atomic<bool>& stop_generators);
    void Notify_all_cv_not_full();

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
};
