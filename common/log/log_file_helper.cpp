#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log/log_file_helper.h"
#include "log/log_cfg_bridge.h"
#include "base/time_utility.h"
namespace Astra {

std::string LogFileHelper::GetSelfUnitName() {
    char path[128] = {0};
    char link[PATH_MAX] = {0};

    snprintf(path, sizeof(path), "/proc/%d/exe", getpid());
    readlink(path, link, sizeof(link) - 1);

    char* bin_name = strrchr(link, '/');
    if (bin_name != NULL) {
        snprintf(path, sizeof(path), "%s", bin_name + 1);
    } else {
        snprintf(path, sizeof(path), "%s", link);
    }
    
    std::string file_name(path);
    return file_name;
}

bool LogFileHelper::GetDateFormatStr(char* str, int len, struct timeval* time_ptr) {
    if (nullptr == str || nullptr == time) {
        return false;
    }

    char date_time_str[MAX_DATE_TIEM_STR];
    TimeUtility::GetDateTimeStr(time_ptr->tv_sec, date_time_str, MAX_DATE_TIEM_STR);
    snprintf(str, len, "[%s.%6ld] ", date_time_str, time_ptr->tv_usec);

    return true;
}

int LogFileHelper::Log(int log_priority, const char* format, ...) {
    char file_name[MAX_FILE_NAME] = {0};
    if (!MakeFileName(file_name, MAX_FILE_NAME)) {
        return -1;
    }

    ShiftFile(file_name);

    va_list ap;
    FILE* file_ptr = fopen(file_name, "a+");
    if (NULL == file_ptr) {
        return -1;
    }

    va_start(ap, format);
    vfprintf(file_ptr, format, ap);
    fprintf(file_ptr, "\n");
    va_end(ap);
    fclose(file_ptr);

    return 0;
}

bool LogFileHelper::MakeFileName(char* str, int len) {
    const LogCfg* logcfg_ptr = LogCfgBridge::Instance()->GetLogCfg();
    if (nullptr == logcfg_ptr) {
        return false;
    }

    snprintf(str, len, "%s/%s", logcfg_ptr->path_name.c_str(), GetSelfUnitName().c_str());

    int format_len = strnlen(str, len);
    if (format_len >= len) {
        return false;
    }

    switch (logcfg_ptr->log_file_cfg.shift_type) {
        case kShiftSize: {
            snprintf(str + format_len, len - format_len, ".log");
            break;
        }
        case kShiftHour: {
            snprintf(str + format_len, len - format_len, "_%u.log", TimeUtility::NowDateHourInt());
            break;
        }
        case kShiftDate: {
            snprintf(str + format_len, len - format_len, "_%u.log", TimeUtility::NowDateInt());
            break;
        }
        default: { 
            return false; 
        }
    }

    str[len] = 0;
    return true;
}

bool LogFileHelper::ShiftFile(const char* filename) {
    const LogCfg* logcfg_ptr = LogCfgBridge::Instance()->GetLogCfg();
    if (nullptr == logcfg_ptr) {
        return false;
    }

    if (NULL == filename || logcfg_ptr->log_file_cfg.max_file_num < 0 ||
        logcfg_ptr->log_file_cfg.max_file_size < 0) {
        return false;
    }

    struct stat file_stat;
    if (stat(filename, &file_stat) < 0) {
        return false;
    }

    if (file_stat.st_size < logcfg_ptr->log_file_cfg.max_file_size) {
        return true;
    }

    // if larger than MaxLogSize
    char log_name[MAX_FILE_NAME] = {0};
    ShiftFileName(log_name, sizeof(log_name), filename, logcfg_ptr->log_file_cfg.max_file_num - 1);
    if (access(log_name, F_OK) == 0) {
        if (remove(log_name) < 0) {
            return false;
        }
    }

    char new_logfile_name[MAX_FILE_NAME] = {0};

    for (int i = logcfg_ptr->log_file_cfg.max_file_num - 2; i >= 0; i--) {
        ShiftFileName(log_name, sizeof(log_name), filename, i);

        if (access(log_name, F_OK) == 0) {
            ShiftFileName(new_logfile_name, sizeof(new_logfile_name), filename, i + 1);

            if (rename(log_name, new_logfile_name) < 0) {
                return false;
            }
        }
    }

    return true;
}

void LogFileHelper::ShiftFileName(char* str, int len, const char* base_name, int num) {
    if (0 != num) {
        snprintf(str, len, "%s%d", base_name, num);
    } else {
        snprintf(str, len, "%s", base_name);
    }

    str[len - 1] = 0;
    return;
}

}  // namespace Astra