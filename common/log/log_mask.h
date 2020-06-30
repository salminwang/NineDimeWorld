#pragma once
#include <stdint.h>
namespace Astra
{

	class LogIDMask
	{
	public:
		static bool IsLogPermit(int log_priority);
	};
} // namespace Astra