#pragma once

#include <stdint.h>
#include <string.h>
#include <string>

namespace Astra
{

	enum LogFileShiftType
	{
		kMinShiftType = -1,
		kShiftSize = 0,
		kShiftHour = 1,
		kShiftDate = 2,
		kMaxShiftType = 3,
	};

	struct LogFileCfg
	{
		LogFileCfg()
		{
			Clear();
		}

		void Clear()
		{
			memset(this, 0, sizeof(*this));
		}
		int shift_type;
		int max_file_size;
		int max_file_num;
	};

	struct LogCfg
	{
		LogCfg()
		{
			Clear();
		}

		void Clear()
		{
			log_priority = 0;
			path_name.clear();
			log_file_cfg.Clear();
		}

		int log_priority;		 //日志的级别
		LogFileCfg log_file_cfg; //日志文件配置
		std::string path_name;
	};
} // namespace Astra