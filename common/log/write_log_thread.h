#pragma once

#include <pthread.h>
#include <string.h>
#include "log/log_msg_define.h"
#include "base/rwlock_helper.h"
#include "log/log_file_helper.h"
namespace Astra
{

#define MAX_THREAD_LOG_NUM 32
#define MAX_SIGNALMASK_NUM 20
#define MAX_TRY_NUM_ON_QUIT 4

#define SLEEP_MS 10
#define MAX_TIME_STR 100

	class LogBufferedMgr;

	// 线程信号掩码屏蔽
	struct ThreadSignalMask
	{
		int signal_num;
		int signals[MAX_SIGNALMASK_NUM];

		ThreadSignalMask() : signal_num(0)
		{
		}

		bool Add(int sig_no)
		{
			if (signal_num >= MAX_SIGNALMASK_NUM)
			{
				assert(false);
				return false;
			}

			signals[signal_num++] = sig_no;
			return true;
		}
	};

	//日志写线程
	class WriteLogThread
	{
	public:
		WriteLogThread() : m_used_thread(false), m_log_thread_id(0), m_thread_num(0)
		{
			memset(m_thread_queue, 0, sizeof(m_thread_queue));
		}

		~WriteLogThread()
		{
		}

		bool init();
		bool AddMsgQueue(ThreadQueue *thread_queue_ptr);
		bool CreateLogThread(ThreadSignalMask *thread_signal_mask_ptr = NULL);
		bool CloseLogThread();

		//从线程管道读
		static void *ThreadFunc(void *p);
		void ThreadRun();

	private:
		int ProcessQueueEvent();
		int ProcessMsg(ThreadMsg &msg, int queue_type);

	private:
		bool m_used_thread;
		pthread_t m_log_thread_id;
		int m_thread_num;
		ThreadQueue *m_thread_queue[MAX_THREAD_LOG_NUM];
		Lock m_lock;
		ThreadSignalMask m_ThreadSignalMask;
		LogFileHelper m_stLogFileHelper;
	};

} // namespace Astra