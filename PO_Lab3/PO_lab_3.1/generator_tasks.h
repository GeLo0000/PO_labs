#pragma once

#include "task_queue.h"
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>

class generator_tasks {
public:
    generator_tasks(task_queue& queue,
        std::condition_variable& not_full,
        std::condition_variable& not_empty,
        std::mutex& cv_mutex,
        int num_threads);
    ~generator_tasks();

    void start();
    void stop();

private:
    void generate_loop(int thread_id);

    task_queue& m_task_queue;
    std::condition_variable& m_not_full_cv;
    std::condition_variable& m_not_empty_cv;
    std::mutex& m_cv_mutex;

    std::vector<std::thread> m_threads;
    std::atomic<bool> m_stop;
    int m_num_threads;
    std::atomic<int> m_task_id_counter;
};
