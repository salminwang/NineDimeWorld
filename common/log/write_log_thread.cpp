#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "write_log_thread.h"
#include "log_cfg_bridge.h"
namespace Astra
{

bool ThreadMaskSignal(int* signals_ptr, int signal_num) {
	if (!signals_ptr || signal_num <= 0) {
		assert(false);
		return  false;
	}

	sigset_t sig_set;
	if (sigemptyset(&sig_set) != 0) {
		return  false;
	}

	for (int i = 0; i < signal_num; i++) {
		sigaddset(&sig_set, signals_ptr[i]);
	}

	if (pthread_sigmask(SIG_BLOCK, &sig_set, nullptr) == 0) {
		return  true;
	}

	return  false;
}

int MsSleep(unsigned int ms) {
	struct timeval tv;

	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	select(0, nullptr, nullptr, nullptr, &tv);
	return 0;
}

void* WriteLogThread::ThreadFunc(void * p) {
	WriteLogThread* log_thread_ptr = (WriteLogThread*)p;
	log_thread_ptr->ThreadRun();

	return nullptr;
}

bool WriteLogThread::init() {
	m_lock.Init();
	return true;
}

bool WriteLogThread::AddMsgQueue(ThreadQueue * thread_queue_ptr) {
	// 可能多线程同时写,依然锁住
	m_lock.AddLock();

	if (m_thread_num >= MAX_THREAD_LOG_NUM) {
		m_lock.Unlock();
		return false;
	}

	m_thread_queue[m_thread_num] = thread_queue_ptr;
	m_thread_num += 1;
	m_lock.Unlock();

	return true;
}

bool WriteLogThread::CreateLogThread(ThreadSignalMask* thread_signal_mask_ptr /*= NULL*/) {
	m_used_thread = true;
	if (NULL != thread_signal_mask_ptr) {
		m_ThreadSignalMask = *thread_signal_mask_ptr;
	}

	pthread_create(&m_log_thread_id, NULL, WriteLogThread::ThreadFunc, (void*)this);
	return  true;
}

void WriteLogThread::ThreadRun() {
	// 线程阻塞信号
	if (m_ThreadSignalMask.signal_num > 0) {
		ThreadMaskSignal(m_ThreadSignalMask.signals, m_ThreadSignalMask.signal_num);
	}

	while (m_used_thread) {
		LogCfgBridge::Instance()->Update();
		int msg_num = ProcessQueueEvent();
		if (0 == msg_num) {
			MsSleep(SLEEP_MS);
		}
	}

	for (int i = 0; i < MAX_TRY_NUM_ON_QUIT; ++i) {
		int num = ProcessQueueEvent();
		if (0 == num)
		{
			break;
		}
	}
}

bool WriteLogThread::CloseLogThread() {
	if (!m_used_thread) {
		return  false;
	}

	if (0 == m_log_thread_id) {
		return false;
	}

	m_used_thread = false;
	void *poThreadRet = nullptr;
	pthread_join(m_log_thread_id, &poThreadRet);

	return  true;
}

int WriteLogThread::ProcessQueueEvent() {
	int proc_msg_Num = 0;

	int thread_num = m_thread_num;
	for (int i = 0; i < thread_num && i < MAX_THREAD_LOG_NUM; ++i) {
		ThreadQueue* thread_queue_ptr = m_thread_queue[i];
		int msg_num = thread_queue_ptr->thread_msg_queue.GetMsgNum();

		for (int j = 0; j < msg_num; j++) {
			ThreadMsg* thread_msg_ptr = thread_queue_ptr->thread_msg_queue.Read();
			if (nullptr == thread_msg_ptr) {
				break;
			}

			ProcessMsg(*thread_msg_ptr, thread_queue_ptr->queue_type);

			++proc_msg_Num;
			thread_queue_ptr->thread_msg_queue.PopHead();
		}
	}

	return proc_msg_Num;
}

int WriteLogThread::ProcessMsg(ThreadMsg& msg, int queue_type) {
	switch (msg.log_msg_type) {
	case THREADMSG_LOG: {
		WriteLogMsgInfo& msg_info = msg.log_msg_info.write_log_msg_info;
		if (0 == msg_info.log_time.tv_sec) {
			gettimeofday(&msg_info.log_time, nullptr);
		}

		char time_str[MAX_TIME_STR] = { 0 };
		Singleton<LogFileHelper>::Instance()->GetDateFormatStr(time_str, MAX_TIME_STR, &msg_info.log_time);
		Singleton<LogFileHelper>::Instance()->Log(msg_info.log_priority, "%s%s", time_str, msg_info.log_content);
		break;
	}
	default: {
		assert(0);
		break;
	}

	}

	return 0;

}
}