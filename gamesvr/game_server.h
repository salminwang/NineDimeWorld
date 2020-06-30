#pragma onece

#include "framework/base_server.h"

namespace Astra
{

    class GameServer : public BaseServer
    {
    public:
        GameServer() : BaseServer("gamesvr")
        {
        }
    };
} // namespace Astra