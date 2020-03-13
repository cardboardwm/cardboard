#ifndef __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
#define __CARDBOARD_IPC_HANDLERS_HANDLERS_H_

#include "../IPC.h"

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
    auto it = server->tiles.find_tile(server->get_focused_view());
    if (it != server->tiles.tiles.end() && std::prev(it) != server->tiles.tiles.end()) {
        std::advance(it, -1);
        server->tiles.fit_view_on_screen(it->view);
        server->focus_view(it->view);
    }
    return { "" };
}

static IPCCommandResult command_focus_right([[maybe_unused]] ParsedCommand cmd, Server* server)
{
    auto it = server->tiles.find_tile(server->get_focused_view());
    if (it != server->tiles.tiles.end() && std::next(it) != server->tiles.tiles.end()) {
        std::advance(it, +1);
        server->tiles.fit_view_on_screen(it->view);
        server->focus_view(it->view);
    }
    return { "" };
}

};

#endif // __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
