#pragma once
#include "base/singleton.h"
#include "log/log_config.h"
#include "tinyxml/include/tinyxml.h"

namespace Astra
{

#define LOGCFG_PATH "../cfg/LogCfg.xml"

    class LogCfgMgr : public Singleton<LogCfgMgr>
    {
    public:
        bool LoadCfg();
        const LogCfg &GetLogCfg()
        {
            return m_log_cfg;
        }

    private:
        bool ParsePriorityStr(const char *str, uint32_t len, int &log_priority) const;
        bool LoadLogFileCfg(TiXmlElement *elem, LogFileCfg &log_file_cfg) const;

    private:
        LogCfg m_log_cfg;
    };
} // namespace Astra