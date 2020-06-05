#pragma once

#include "log/log_msg_define.h"
#include "log/log_msg_queue.h"
#include "log/log_mask.h"
#include "base/singleton.h"
namespace Astra
{

#define LOG(LOGTYPE, _s_, ...)\
do {\
	if (!Singleton<LogIDMask>::Instance()->IsLogPermit(LOGTYPE)) {\
		break;\
	}\
	Singleton<LogMsgQueue, true>::Instance()->Log(LOGTYPE, NULL, "[%s][%s][%d][%s] " _s_, #LOGTYPE, __FILE__,\
                __LINE__, __FUNCTION__, ##__VA_ARGS__);\
}while (0);


#define AS_INFO(_s_, ...)     LOG(INFO, _s_, ##__VA_ARGS__)
#define AS_DEBUG(_s_, ...)    LOG(DEBUG_P, _s_, ##__VA_ARGS__)
#define AS_SYS(_s_, ...)      LOG(SYS, _s_, ##__VA_ARGS__)
#define AS_WARNING(_s_, ...)  LOG(WARNING, _s_, ##__VA_ARGS__)
#define AS_WARN(_s_, ...)     LOG(WARNING, _s_, ##__VA_ARGS__)
#define AS_ERROR(_s_, ...)    LOG(ERROR_P, _s_, ##__VA_ARGS__)
#define AS_FATAL(_s_, ...)    LOG(FATAL, _s_, ##__VA_ARGS__)
}