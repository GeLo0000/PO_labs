#pragma once

#include <queue>
#include <thread>
#include <shared_mutex>
#include "task.h"

using read_write_lock = std::shared_mutex;
using read_lock = std::shared_lock<read_write_lock>;
using write_lock = std::unique_lock<read_write_lock>;

class task_queue {
public:
	task_queue() = default;
	~task_queue() { clear(); }
	bool empty() const;
	size_t size() const;

public:
	void clear();
	bool pop(task& save_task);
	bool emplace(int id, int time);

private:
	mutable read_write_lock m_rw_lock;
	std::queue<task> m_tasks;
};