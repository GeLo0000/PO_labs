#include "thread_pool.h"
#include <iostream>
#include <chrono>

thread_pool::thread_pool(task_queue& queue,
    std::condition_variable& not_empty,
    std::condition_variable& not_full,
    std::mutex& cv_mutex,
    int num_threads)
    : m_task_queue(queue),
    m_not_empty_cv(not_empty),
    m_not_full_cv(not_full),
    m_cv_mutex(cv_mutex),
    m_num_threads(num_threads),
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
            // Чекаємо, поки черга не буде порожньою або не прийде сигнал завершення
            m_not_empty_cv.wait(lock, [this, thread_id]() {
                std::cout << "[Consumer " << thread_id << "] Try for task...\n";
                return !m_task_queue.empty() || m_stop;
                });

            // Якщо пауза — чекаємо знову
            while (m_paused && !m_stop) {
                m_not_empty_cv.wait(lock);
            }

            if (m_stop) break;

            // Пробуємо взяти задачу
            if (!m_task_queue.pop(task_to_process)) {
                continue;
            }

            // Сповіщаємо генераторів, що з'явилось місце
            m_not_full_cv.notify_one();

            // Після цього ми можемо звільнити м’ютекс — обробка задачі не вимагає синхронізації
            //lock.unlock();
        }

        // Обробка задачі
        std::cout << "[Consumer " << thread_id << "] Start Processing task " << task_to_process.get_id() << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(task_to_process.get_time()));

        std::cout << "[Consumer " << thread_id << "] Finish Processing task " << task_to_process.get_id() << "\n";
    }
}