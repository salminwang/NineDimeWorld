#pragma once
#include <stdint.h>
#include <string>
#include "log/log_msg_define.h"

namespace Astra
{

#define MAX_FILE_NAME 100
#define MAX_DATE_TIEM_STR 50

	class LogFileHelper
	{
	public:
		bool GetDateFormatStr(char *str, int len, struct timeval *time);
		int Log(int log_priority, const char *format, ...);
		bool MakeFileName(char *str, int iLen);

	private:
		std::string GetSelfUnitName();
		bool ShiftFile(const char *filename);
		void ShiftFileName(char *str, int len, const char *base_name, int num);
	};

} // namespace Astra