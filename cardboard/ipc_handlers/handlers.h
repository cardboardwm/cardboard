#ifndef __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
#define __CARDBOARD_IPC_HANDLERS_HANDLERS_H_

#include "../IPC.h"
#include "../Server.h"

namespace ipc_handlers {

static IPCCommandResult command_quit(ParsedCommand cmd, Server* server)
{
    int exit = 0;
    if (cmd.size() >= 2) {
        exit = std::atoi(cmd[1].c_str());
    }
    server->teardown(exit);
    return { "" };
}

static IPCCommandResult command_focus_left([[maybe_unused]] ParsedCommand cmd, Server* server)
{
    server->focus_by_offset(-1);
    return { "" };
}

static IPCCommandResult command_focus_right([[maybe_unused]] ParsedCommand cmd, Server* server)
{
    server->focus_by_offset(+1);
    return { "" };
}

};

#endif // __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
