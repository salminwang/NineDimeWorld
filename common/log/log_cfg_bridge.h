#pragma once

#include <memory>
#include "log/log_config.h"
#include "base/singleton.h"
#include "base/rwlock_helper.h"

namespace Astra
{

	typedef std::shared_ptr<LogCfg> LogConfigSharedPtr;

	class LogCfgBridgeInstance : public Singleton<LogCfgBridgeInstance>
	{
	public:
		LogCfgBridgeInstance();
		void Update(const LogCfg &log_cfg);
		LogConfigSharedPtr GetLogBasePtr();

	private:
		LogConfigSharedPtr m_logcfg_Ptr;
		RWLock m_rwlock;
	};

	class LogCfgBridge : public Singleton<LogCfgBridge, true>
	{
	public:
		LogCfgBridge();
		void Update();
		uint32_t GetPriority();
		const LogCfg *GetLogCfg();

	private:
		LogConfigSharedPtr m_logcfg_Ptr;
		RWLock m_rwlock;
	};
} // namespace Astra