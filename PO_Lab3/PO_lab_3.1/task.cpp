#include "task.h"
#include <iostream>

task::task(int id, int time) {
	m_id = id;
	m_time = time;
}

task::task() {
	m_id = 0;
	m_time = 0;
}

task::~task(){
	
}

int task::get_id() const{
	return m_id;
}

int task::get_time() const {
	return m_time;
}
