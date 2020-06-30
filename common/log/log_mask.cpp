#include "log/log_mask.h"
#include "log/log_cfg_bridge.h"

namespace Astra
{

	bool LogIDMask::IsLogPermit(int log_priority)
	{
		if (log_priority <= 0)
		{
			assert(0);
			return false;
		}

		uint32_t priority = LogCfgBridge::Instance()->GetPriority();
		if (log_priority & priority)
		{
			return true;
		}

		return false;
	}
} // namespace Astra