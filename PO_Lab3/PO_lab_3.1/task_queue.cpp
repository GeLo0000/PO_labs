#include "task_queue.h"
#include <iostream>


bool task_queue::empty() const
{
	read_lock _(m_rw_lock);
	return m_tasks.empty();
}

size_t task_queue::size() const
{
	read_lock _(m_rw_lock);
	return m_tasks.size();
}

void task_queue::clear()
{
	write_lock _(m_rw_lock);
	while (!m_tasks.empty())
	{
		m_tasks.pop();
	}
}

bool task_queue::pop(task& save_task)
{
	write_lock _(m_rw_lock);
	if (m_tasks.empty())
	{
		return false;
	}
	else
	{
		save_task = m_tasks.front();
		m_tasks.pop();
		return true;
	}
}

bool task_queue::emplace(int id, int time)
{
	write_lock _(m_rw_lock);
	if (m_tasks.size() >= 20)
	{
		return false;
	}
	else
	{
		m_tasks.emplace(id, time);
		return true;
	}
}