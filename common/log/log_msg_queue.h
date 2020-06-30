#pragma once
#include "log/log_msg_define.h"
#include <time.h>

namespace Astra
{

	class LogMsgQueue
	{
	public:
		LogMsgQueue() : m_thread_queue(NULL)
		{
		}

		bool CreateMsgQueue(int queue_type, int max_log_msg_num);
		bool Log(int log_priority, struct timeval *time_ptr, const char *fmt, ...);

	private:
		ThreadQueue *m_thread_queue;
	};

} // namespace Astra