#include "thread_pool.h"
#include <iostream>
#include <chrono>

thread_pool::thread_pool(int num_threads)
    : m_num_threads(num_threads),
    m_stop(false),
    m_paused(false) {
}

thread_pool::~thread_pool() {
    stop();
}

void thread_pool::start() {
    for (int i = 0; i < m_num_threads; ++i) {
        m_threads.emplace_back(&thread_pool::worker_loop, this, i);
    }
}

void thread_pool::stop() {
    m_stop = true;
    m_not_empty_cv.notify_all();
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void thread_pool::pause() {
    m_paused = true;
}

void thread_pool::resume() {
    m_paused = false;
    m_not_empty_cv.notify_all();
}

void thread_pool::worker_loop(int thread_id) {
    while (!m_stop) {
        task task_to_process;

        {
            std::unique_lock<std::mutex> lock(m_cv_mutex);
            std::cout << "[Consumer " << thread_id << "] Waiting for task...\n";

            m_not_empty_cv.wait(lock, [this, thread_id]() {
                return !m_task_queue.empty() || m_stop;
                });

            while (m_paused && !m_stop) {
                m_not_empty_cv.wait(lock);
            }

            if (m_stop) break;

            if (!m_task_queue.pop(task_to_process)) {
                continue;
            }

            m_not_full_cv.notify_one();

            //lock.unlock();
        }

        std::cout << "[Consumer " << thread_id << "] Start Processing task " << task_to_process.get_id() << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(task_to_process.get_time()));

        std::cout << "[Consumer " << thread_id << "] Finish Processing task " << task_to_process.get_id() << "\n";
    }
}

void thread_pool::add_task(int thread_id, std::atomic<bool>& stop_generators) {

    {
        std::unique_lock<std::mutex> lock(m_cv_mutex);
        m_not_full_cv.wait(lock, [this, &stop_generators]() {
            return m_task_queue.size() < 20 || stop_generators.load(); //TODO не забути обробити видалення
        });
    }

    if (stop_generators) return;

    int task_id = m_task_id_counter++;
    int time = rand() % 6 + 5;
    if (m_task_queue.emplace(task_id, time)) {
        std::cout << "[Generator " << thread_id << "] Added task " << task_id << "\n";
        m_not_empty_cv.notify_one();
    }
}

void thread_pool::Notify_all_cv_not_full() {
    m_not_full_cv.notify_all();
}
