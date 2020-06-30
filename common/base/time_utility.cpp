#include "base/time_utility.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

namespace Astra
{
    int64_t TimezoneHelper::timezone_diff_secs = 0;

    int64_t TimeUtility::GetCurrentMS()
    {
        int64_t timestamp = GetCurrentUS();
        return timestamp / 1000;
    }

    int64_t TimeUtility::GetCurrentUS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        int64_t timestamp = tv.tv_sec * 1000000 + tv.tv_usec;
        return timestamp;
    }

    std::string TimeUtility::GetStringTime()
    {
        time_t now = time(NULL);

        struct tm tm_now;
        struct tm *p_tm_now;

        p_tm_now = localtime_r(&now, &tm_now);

        char buff[256] = {0};
        snprintf(buff, sizeof(buff), "%04d-%02d-%02d% 02d:%02d:%02d",
                 1900 + p_tm_now->tm_year, p_tm_now->tm_mon + 1, p_tm_now->tm_mday,
                 p_tm_now->tm_hour, p_tm_now->tm_min, p_tm_now->tm_sec);

        return std::string(buff);
    }

    const char *TimeUtility::GetStringTimeDetail(bool reset_time)
    {
        static char buff[64] = {0};
        static struct timeval tv_now;
        static time_t now;
        static struct tm tm_now;
        static struct tm *p_tm_now;

        if (!reset_time && (buff[0] != '\0'))
        {
            return buff;
        }

        gettimeofday(&tv_now, NULL);
        now = (time_t)tv_now.tv_sec;
        p_tm_now = localtime_r(&now, &tm_now);

        snprintf(buff, sizeof(buff), "%04d-%02d-%02d %02d:%02d:%02d.%06d",
                 1900 + p_tm_now->tm_year, p_tm_now->tm_mon + 1, p_tm_now->tm_mday,
                 p_tm_now->tm_hour, p_tm_now->tm_min, p_tm_now->tm_sec,
                 static_cast<int>(tv_now.tv_usec));

        return buff;
    }

    time_t TimeUtility::GetTimeStamp(const std::string &time)
    {
        tm tm_;
        char buf[128] = {0};
        strncpy(buf, time.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = 0;
        strptime(buf, "%Y-%m-%d %H:%M:%S", &tm_);
        tm_.tm_isdst = -1;
        return mktime(&tm_);
    }

    time_t TimeUtility::GetTimeDiff(const std::string &t1, const std::string &t2)
    {
        time_t time1 = GetTimeStamp(t1);
        time_t time2 = GetTimeStamp(t2);
        time_t time = time1 - time2;
        return time;
    }

    void TimeUtility::FastParseDateTime(time_t tTime, int &iYear, int &iMonth, int &iDay, int &iHour, int &iMin, int &iSec, int time_zone /*8*/)
    {
        tm tm;
        static const int kHoursInDay = 24;
        static const int kMinutesInHour = 60;
        static const int kDaysFromUnixTime = 2472632;
        static const int kDaysFromYear = 153;
        static const int kMagicUnkonwnFirst = 146097;
        static const int kMagicUnkonwnSec = 1461;
        tm.tm_sec = tTime % kMinutesInHour;
        int i = (tTime / kMinutesInHour);
        tm.tm_min = i % kMinutesInHour; // nn
        i /= kMinutesInHour;
        tm.tm_hour = (i + time_zone) % kHoursInDay; // hh
        tm.tm_mday = (i + time_zone) / kHoursInDay;
        int a = tm.tm_mday + kDaysFromUnixTime;
        int b = (a * 4 + 3) / kMagicUnkonwnFirst;
        int c = (-b * kMagicUnkonwnFirst) / 4 + a;
        int d = ((c * 4 + 3) / kMagicUnkonwnSec);
        int e = -d * kMagicUnkonwnSec;
        e = e / 4 + c;
        int m = (5 * e + 2) / kDaysFromYear;
        tm.tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
        tm.tm_mon = (-m / 10) * 12 + m + 2;
        tm.tm_year = b * 100 + d - 6700 + (m / 10);

        iYear = tm.tm_year + 1900;
        iMonth = tm.tm_mon + 1;
        iDay = tm.tm_mday;
        iHour = tm.tm_hour;
        iMin = tm.tm_min;
        iSec = tm.tm_sec;
    }

    void TimeUtility::ParseDate(time_t tTime, int &iYear, int &iMonth, int &iDay)
    {
        int iHour, iMin, iSec;
        FastParseDateTime(tTime, iYear, iMonth, iDay, iHour, iMin, iSec);
    }

    int TimeUtility::GetDateAsInt(time_t time)
    {
        int year, month, day;
        ParseDate(time, year, month, day);
        return year * 10000 + month * 100 + day;
    }

    int TimeUtility::GetTimeAsInt(time_t time)
    {
        int hour, minute, second;
        ParseTime(time, hour, minute, second);
        return hour * 10000 + minute * 100 + second;
    }

    void TimeUtility::ParseTime(time_t time, int &hour, int &minute, int &second)
    {
        int year, month, day;
        ParseDateTime(time, year, month, day, hour, minute, second);
    }

    void TimeUtility::ParseDateTime(time_t time, int &year, int &month, int &day, int &hour, int &minute, int &second)
    {
        tm tm;
        localtime_r(&time, &tm);
        year = tm.tm_year + 1900;
        month = tm.tm_mon + 1;
        day = tm.tm_mday;
        hour = tm.tm_hour;
        minute = tm.tm_min;
        second = tm.tm_sec;
    }

    const char *TimeUtility::GetDateTimeStr(time_t time, char *buff, size_t buff_len)
    {
        tm tm;
        tm2time(time, tm);
        GetDateTimeStr(tm, buff, buff_len);
        return buff;
    }

    const char *TimeUtility::GetDateTimeStr(tm &tm, char *buff, size_t buff_len)
    {
        snprintf(buff, buff_len, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        return buff;
    }

    void TimeUtility::tm2time(const time_t &t, struct tm &tt)
    {
        //加快速度, 否则会比较慢, 不用localtime_r(会访问时区文件, 较慢)
        static TimezoneHelper helper;
        time_t localt = t + TimezoneHelper::timezone_diff_secs;

        gmtime_r(&localt, &tt);
    }

    uint32_t TimeUtility::NowDateHourInt()
    {
        static struct timeval time_val;
        gettimeofday(&time_val, NULL);
        return GetDateAsInt(time_val.tv_sec) * 100 + GetTimeAsInt(time_val.tv_sec) / 10000 % 100;
    }

    uint32_t TimeUtility::NowDateInt()
    {
        static struct timeval time_val;
        gettimeofday(&time_val, NULL);
        return GetDateAsInt(time_val.tv_sec);
    }

    HourGlassMS::HourGlassMS()
    {
        m_begin_time = TimeUtility::GetCurrentMS();
    }

    void HourGlassMS::Reset()
    {
        m_begin_time = TimeUtility::GetCurrentMS();
    }

    time_t HourGlassMS::Elapse()
    {
        return TimeUtility::GetCurrentMS() - m_begin_time;
    }

} // namespace Astra