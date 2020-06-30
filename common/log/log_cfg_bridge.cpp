#include "log/log_cfg_bridge.h"

namespace Astra
{

	LogCfgBridge::LogCfgBridge()
	{
	}

	LogCfgBridgeInstance::LogCfgBridgeInstance()
	{
		m_rwlock.Init();
	}

	void LogCfgBridgeInstance::Update(const LogCfg &log_cfg)
	{
		LogConfigSharedPtr pSharedPtr(new LogCfg());
		m_rwlock.WLock();
		m_logcfg_Ptr = pSharedPtr;
		m_logcfg_Ptr->log_priority = log_cfg.log_priority;
		m_logcfg_Ptr->log_file_cfg = log_cfg.log_file_cfg;
		m_logcfg_Ptr->path_name = log_cfg.path_name;
		m_rwlock.Unlock();
	}

	LogConfigSharedPtr LogCfgBridgeInstance::GetLogBasePtr()
	{
		m_rwlock.RLock();
		LogConfigSharedPtr logcfg_ptr = m_logcfg_Ptr;
		m_rwlock.Unlock();

		return logcfg_ptr;
	}

	void LogCfgBridge::Update()
	{
		LogConfigSharedPtr logcfg_ptr = LogCfgBridgeInstance::Instance()->GetLogBasePtr();
		m_logcfg_Ptr = logcfg_ptr;
	}

	uint32_t LogCfgBridge::GetPriority()
	{
		LogConfigSharedPtr logcfg_ptr = m_logcfg_Ptr;
		return logcfg_ptr->log_priority;
	}

	const LogCfg *LogCfgBridge::GetLogCfg()
	{
		return &(*m_logcfg_Ptr);
	}
} // namespace Astra