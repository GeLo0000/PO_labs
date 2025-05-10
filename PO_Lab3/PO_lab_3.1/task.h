#pragma once

class task { 
public:
	task(int id, int time);
	task();
	~task();
	
	int get_id() const;
	int get_time() const;

private:
	int m_id;
	int m_time;
};