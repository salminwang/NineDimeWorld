#pragma once

#include <time.h>
#include <stdint.h>
#include "base/heap_obj_ring_queue.h"

namespace Astra
{

	const int kBuffLogLen = 204800;
	const int kMaxLogMsgNum = 500;

	//日志级别
	enum LogPriority
	{
		INFO = 1,
		DEBUG_P = INFO << 1,
		WARNING = INFO << 2,
		SYS = INFO << 3,
		ERROR_P = INFO << 4,
		FATAL = INFO << 5,
	};

	enum LogMsgType
	{
		THREADMSG_LOG = 0, // 日志记录
	};

	struct WriteLogMsgInfo
	{
		int log_priority;
		char log_content[kBuffLogLen];
		struct timeval log_time;
	};

	union UNLogMsgInfo {
		WriteLogMsgInfo write_log_msg_info;
	};

	struct ThreadMsg
	{
		LogMsgType log_msg_type;
		UNLogMsgInfo log_msg_info;
	};

	struct ThreadQueue
	{
		//管道类型
		int queue_type;
		HeapObjRingQueue<ThreadMsg> thread_msg_queue;
	};

} // namespace Astra