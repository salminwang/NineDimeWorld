#pragma once

#include <assert.h>
#include <stdint.h>

template<typename OBJTYPE>
class HeapObjRingQueue {
public:
	HeapObjRingQueue() :m_max_obj_num(0), m_obj_root(NULL), m_obj_head(0), m_obj_tail(0)
	{}

	~HeapObjRingQueue() {
		if (m_obj_root) {
			delete[] m_obj_root;
			m_obj_root = NULL;
		}
	}

	bool    Init(uint32_t max);
	bool    Write(const OBJTYPE &obj);
	OBJTYPE*    Read();
	bool    PopHead();
	int    GetMsgNum();

private:
	uint32_t    m_max_obj_num;
	OBJTYPE*    m_obj_root;

	uint32_t    m_obj_head;
	uint32_t    m_obj_tail;

};

template<typename OBJTYPE>
bool HeapObjRingQueue<OBJTYPE>::Init(uint32_t max) {
	if (max == 0) {
		assert(false);
		return  false;
	}

	m_obj_root = new OBJTYPE[max];
	m_max_obj_num = max;

	m_obj_head = 0;
	m_obj_tail = 0;

	return  true;
}

template<typename OBJTYPE>
int HeapObjRingQueue<OBJTYPE>::GetMsgNum() {
	if (0 == m_max_obj_num) {
		return 0;
	}

	return (m_obj_tail - m_obj_head + m_max_obj_num) % m_max_obj_num;
}

template<typename OBJTYPE>
bool HeapObjRingQueue<OBJTYPE>::Write(const OBJTYPE &obj) {
	uint32_t    head_pos = m_obj_head;
	uint32_t    next_tail_pos = (m_obj_tail + 1) % m_max_obj_num;

	if (next_tail_pos == head_pos) {
		return  false;
	}

	m_obj_root[m_obj_tail] = obj;
	m_obj_tail = next_tail_pos;

	return  true;
}

template<typename OBJTYPE>
OBJTYPE* HeapObjRingQueue<OBJTYPE>::Read() {
	uint32_t    tail_Pos = m_obj_tail;
	if (m_obj_head == tail_Pos) {
		return  NULL;
	}

	return  &m_obj_root[m_obj_head];
}

template<typename OBJTYPE>
bool HeapObjRingQueue<OBJTYPE>::PopHead() {
	uint32_t    tail_Pos = m_obj_tail;
	if (m_obj_head == tail_Pos) {
		return  false;
	}

	uint32_t    next_head_pos = (m_obj_head + 1) % m_max_obj_num;
	m_obj_head = next_head_pos;

	return  true;
}


