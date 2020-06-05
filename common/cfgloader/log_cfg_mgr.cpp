#include <stdint.h>
#include "cfgloader/log_cfg_mgr.h"
#include "base/string_helper.h"
#include "log/log_msg_define.h"

namespace Astra {

#define INFO_STR "INFO"
#define DEBUG_STR "DEBUG"
#define WARNING_STR "WARNING"
#define SYS_STR "SYSTEM"
#define ERROR_STR "ERROR"
#define FATAL_STR "FATAL"

#define LOG_PRIORITY_SPLIT_CHAR '|'
#define MAX_PRIORITY_STR_SIZE 100
#define MAX_PRIORITY_SIZE 10
#define MAX_PATHNAME_SIZE 100

bool LogCfgMgr::LoadCfg() {
    TiXmlDocument document;
    if (!document.LoadFile(LOGCFG_PATH)) {
        return false;
    }

    TiXmlElement* root = document.RootElement();
    if (!root) {
        return false;
    }

    m_log_cfg.Clear();

    TiXmlElement* elem = root->FirstChildElement("BaseLogPath");
    if (nullptr == elem) {
        return false;
    }

    const char* out_text = elem->GetText();
    char path_name[MAX_PATHNAME_SIZE] = {0};
    snprintf(path_name, MAX_PATHNAME_SIZE, "%s", out_text);
    m_log_cfg.path_name = path_name;

    elem = root->FirstChildElement("BaseLogPriority");
    if (nullptr == elem) {
        return false;
    }

    const char* log_priority = elem->GetText();
    if (nullptr == log_priority) {
        return false;
    }

    char tmp[MAX_PRIORITY_STR_SIZE] = {0};
    snprintf(tmp, MAX_PRIORITY_STR_SIZE, "%s", log_priority);

    StringTokenizer string_tonken;
    string_tonken.Begin(LOG_PRIORITY_SPLIT_CHAR, tmp, strnlen(tmp, sizeof(tmp)));

    const char* str = NULL;
    uint32_t len = 0;

    while (string_tonken.Next(str, len)) {
        if (NULL == str || 0 == len) {
            break;
        }

        ParsePriorityStr(str, len, m_log_cfg.log_priority);
    }

    elem = root->FirstChildElement("BaseLogFileCfg");
    if (nullptr == elem) {
        return false;
    }

    if (!LoadLogFileCfg(elem, m_log_cfg.log_file_cfg)) {
        return false;
    }

    return true;
}

bool LogCfgMgr::ParsePriorityStr(const char* str, uint32_t len, int& log_priority) const {
    if (len >= MAX_PRIORITY_SIZE) {
        return false;
    }

    char tmp[MAX_PRIORITY_SIZE] = {0};
    memcpy(tmp, str, len);

    if (!strncmp(tmp, INFO_STR, sizeof(INFO_STR))) {
        log_priority |= INFO;
    } else if (!strncmp(tmp, DEBUG_STR, sizeof(DEBUG_STR))) {
        log_priority |= DEBUG_P;
    } else if (!strncmp(tmp, WARNING_STR, sizeof(WARNING_STR))) {
        log_priority |= WARNING;
    } else if (!strncmp(tmp, SYS_STR, sizeof(SYS_STR))) {
        log_priority |= SYS;
    } else if (!strncmp(tmp, ERROR_STR, sizeof(ERROR_STR))) {
        log_priority |= ERROR_P;
    } else if (!strncmp(tmp, FATAL_STR, sizeof(FATAL_STR))) {
        log_priority |= FATAL;
    }

    return true;
}

bool LogCfgMgr::LoadLogFileCfg(TiXmlElement* elem, LogFileCfg& log_file_cfg) const {
    TiXmlElement* sub_elem = elem->FirstChildElement("ShiftType");
    if (nullptr == sub_elem) {
        return false;
    }

    const char* res_str = sub_elem->GetText();
    log_file_cfg.shift_type = atoi(res_str);

    sub_elem = elem->FirstChildElement("MaxFileSize");
    if (nullptr == sub_elem) {
        return false;
    }

    res_str = sub_elem->GetText();
    log_file_cfg.max_file_size = atoi(res_str);

    sub_elem = elem->FirstChildElement("MaxFileNum");
    if (nullptr == sub_elem) {
        return false;
    }

    res_str = sub_elem->GetText();
    log_file_cfg.max_file_num = atoi(res_str);

    if (log_file_cfg.shift_type >= kMaxShiftType ||
        log_file_cfg.shift_type < kMinShiftType) {
        return false;
    }

    return true;
}

}  // namespace bg