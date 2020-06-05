#include <cstdarg>
#include "base/singleton.h"
#include "log/log_msg_queue.h"
#include "log/write_log_thread.h"
#include "log/log_msg_define.h"

namespace Astra {

bool LogMsgQueue::CreateMsgQueue(int queue_type, int max_log_msg_num) {
	if (m_thread_queue != NULL) {
		return false;
	}

	m_thread_queue = new ThreadQueue;

	m_thread_queue->queue_type = queue_type;
	m_thread_queue->thread_msg_queue.Init(max_log_msg_num);

	return Singleton<WriteLogThread>::Instance()->AddMsgQueue(m_thread_queue);
}

bool LogMsgQueue::Log(int log_priority, struct timeval *time_ptr, const char* fmt, ...) {
	if (NULL == m_thread_queue) {
		return false;
	}

	ThreadMsg msg;
	msg.log_msg_type = THREADMSG_LOG;

	WriteLogMsgInfo& log_msg = msg.log_msg_info.write_log_msg_info;
	log_msg.log_priority = log_priority;

	va_list	ap;
	va_start(ap, fmt);
	vsnprintf(log_msg.log_content, sizeof(log_msg.log_content), fmt, ap);
	va_end(ap);

	if (time_ptr) {
		log_msg.log_time.tv_sec = time_ptr->tv_sec;
		log_msg.log_time.tv_usec = time_ptr->tv_usec;
	} else {
		log_msg.log_time.tv_sec = 0;
		log_msg.log_time.tv_usec = 0;
	}

	return m_thread_queue->thread_msg_queue.Write(msg);

}

}