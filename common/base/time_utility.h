#pragma once
#include <string>

namespace Astra {

class TimezoneHelper {
public:
	TimezoneHelper() {
		struct tm time_info;
		time_t secs, local_secs, gmt_secs;
		time(&secs);

		//带时区时间
        tzset();
        localtime_r(&secs, &time_info);

		local_secs = mktime(&time_info);

		//不带时区时间
        gmtime_r(&secs, &time_info);

		gmt_secs = mktime(&time_info);
		timezone_diff_secs = local_secs - gmt_secs;
	}

	static int64_t timezone_diff_secs;
};

int64_t TimezoneHelper::timezone_diff_secs = 0;

class TimeUtility {
public:
    // 得到当前的毫秒
    static int64_t GetCurrentMS();

    // 得到当前的微妙
    static int64_t GetCurrentUS();

    // 得到字符串形式的时间 格式：2015-04-10 10:11:12
    static std::string GetStringTime();

    // 得到字符串形式的详细时间 格式: 2015-04-10 10:11:12.967151
    static const char* GetStringTimeDetail(bool reset_time = true);

    // 将字符串格式(2015-04-10 10:11:12)的时间，转为time_t(时间戳)
    static time_t GetTimeStamp(const std::string& time);

    // 取得两个时间戳字符串t1-t2的时间差，精确到秒,时间格式为2015-04-10 10:11:12
    static time_t GetTimeDiff(const std::string& t1, const std::string& t2);

    static int GetDateAsInt(time_t time);
    static int GetTimeAsInt(time_t time);

	static const char* GetDateTimeStr(time_t time, char* buff, size_t buff_len);
	static const char* GetDateTimeStr(tm &tm, char* buff, size_t buff_len);

	static void ParseTime(time_t time, int& hour, int& minute, int& second);
	static void ParseDate(time_t time, int& year, int& month, int& day);

	static void ParseDateTime(time_t time, int& year, int& month, int& day, int& hour, int& minute, int& second);
	static void FastParseDateTime(time_t time, int& year, int& month, int& day, int& hour, int& minute, int& second, int time_zone = 8);

	static void tm2time(const time_t &t, struct tm &tt);

    static uint32_t NowDateHourInt();
    static uint32_t NowDateInt();
};

// 沙漏，计算时间消耗辅助类
class HourGlassMS {
public:
    HourGlassMS();

    void Reset();

    // 从记时到现在流逝的时间ms
    time_t Elapse();

private:
    time_t m_begin_time;
};

}