#include "game_server.h"
int main(int argc, const char *argv[])
{
    auto server = new Astra::GameServer();
    server->RunServer(argc - 1, argv + 1);
    return 0;
}