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
    m_wait_times.resize(m_num_threads);
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

        std::chrono::high_resolution_clock::time_point wait_start_time;
        std::chrono::high_resolution_clock::time_point wait_end_time;

        {
            std::unique_lock<std::mutex> lock(m_cv_mutex);
            std::cout << "[Consumer " << thread_id << "] Waiting for task...\n";

            wait_start_time = std::chrono::high_resolution_clock::now();

            m_not_empty_cv.wait(lock, [this, thread_id]() {
                return !m_task_queue.empty() || m_stop;
                });

            wait_end_time = std::chrono::high_resolution_clock::now();

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

        std::chrono::duration<double> wait_duration = wait_end_time - wait_start_time;

        {
            std::lock_guard<std::mutex> lock(m_wait_times_mutex);
            m_wait_times[thread_id].push_back(wait_duration.count());
        }

        std::cout << "[Consumer " << thread_id << "] Start Processing task " << task_to_process.get_id() << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(task_to_process.get_time()));

        std::cout << "[Consumer " << thread_id << "] Finish Processing task " << task_to_process.get_id() << "\n";
    }
}

bool thread_pool::add_task(int thread_id) {

    if (m_task_queue.size() >= 20) {
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(m_cv_mutex);
        m_not_full_cv.wait(lock, [this]() {
            return m_task_queue.size() < 20;
        });
    }

    int task_id = m_task_id_counter++;
    int time = rand() % 6 + 5;
    if (m_task_queue.emplace(task_id, time)) {
        std::cout << "[Generator " << thread_id << "] Added task " << task_id << "\n";
        m_not_empty_cv.notify_one();
        return true;
    }
    else return false;
}

void thread_pool::print_average_wait_times() {
    std::lock_guard<std::mutex> lock(m_wait_times_mutex);

    std::cout << "\n[Stats] Average wait times:\n";

    for (size_t i = 0; i < m_wait_times.size(); ++i) {
        double sum = 0;
        for (double t : m_wait_times[i]) {
            sum += t;
        }
        double avg = m_wait_times[i].empty() ? 0 : sum / m_wait_times[i].size();
        std::cout << "  Thread " << i << ": " << avg << " seconds ("
            << m_wait_times[i].size() << " samples)\n";
    }
}