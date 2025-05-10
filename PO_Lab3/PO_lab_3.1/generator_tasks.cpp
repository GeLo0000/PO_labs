#include "generator_tasks.h"
#include <iostream>
#include <chrono>

generator_tasks::generator_tasks(task_queue& queue,
    std::condition_variable& not_full,
    std::condition_variable& not_empty,
    std::mutex& cv_mutex,
    int num_threads)
    : m_task_queue(queue),
    m_not_full_cv(not_full),
    m_not_empty_cv(not_empty),
    m_cv_mutex(cv_mutex),
    m_num_threads(num_threads),
    m_stop(false),
    m_task_id_counter(0) {
}

generator_tasks::~generator_tasks() {
    stop();
}

void generator_tasks::start() {
    for (int i = 0; i < m_num_threads; ++i) {
        m_threads.emplace_back(&generator_tasks::generate_loop, this, i);
    }
}

void generator_tasks::stop() {
    m_stop = true;
    for (auto& thread : m_threads) {
        if (thread.joinable())
            thread.join();
    }
}

void generator_tasks::generate_loop(int thread_id) {
    while (!m_stop) {
        {
            std::unique_lock<std::mutex> lock(m_cv_mutex);
            m_not_full_cv.wait(lock, [this]() {
                return m_task_queue.size() < 20 || m_stop;
                });
        }

        if (m_stop) break;
        int task_id = m_task_id_counter++;
        if (m_task_queue.emplace(task_id, 5)) {//TODO random from 5 to 10 sec
            std::cout << "[Generator " << thread_id << "] Added task " << task_id << "\n";
            m_not_empty_cv.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
