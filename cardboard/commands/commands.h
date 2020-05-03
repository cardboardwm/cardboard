#ifndef __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
#define __CARDBOARD_IPC_HANDLERS_HANDLERS_H_

#include <csignal>
#include <cstdint>
#include <locale>
#include <string>
#include <string_view>

#include "../Command.h"
#include "../IPC.h"
#include "../Server.h"
#include "../Spawn.h"

extern char** environ;

namespace commands {

inline CommandResult config_mouse_mod(Server* server, uint32_t modifiers)
{
    server->config.mouse_mods = modifiers;
    return { "" };
}

inline CommandResult focus(Server* server, int focus_direction)
{
    server->seat.focus_by_offset(server, focus_direction);
    return { "" };
}

inline CommandResult quit(Server* server, int code)
{
    server->teardown(code);
    return { "" };
}

[[maybe_unused]] static std::array<std::pair<std::string_view, uint32_t>, 8> mod_table = {};

inline CommandResult bind(Server* server, uint32_t modifiers, xkb_keysym_t sym, const Command& command)
{
    server->keybindings_config.map[modifiers].insert({ sym, command });
    return { "" };
}

inline CommandResult exec(Server*, std::vector<std::string> arguments)
{
    spawn([&arguments]() {
        std::vector<char*> argv;
        argv.reserve(arguments.size());
        for (const auto& arg : arguments)
            strcpy(
                argv.emplace_back(static_cast<char*>(malloc(arg.size() + 1))),
                arg.c_str());
        argv.push_back(nullptr);

        int err_code = execvpe(argv[0], argv.data(), environ);

        for (auto p : argv)
            free(p);

        return err_code;
    });

    return { "" };
}

inline CommandResult close(Server* server)
{
    server->seat.get_focused_view()->close();

    return { "" };
}

inline CommandResult workspace_switch(Server* server, int n)
{
    using namespace std::string_literals;

    if(n < 0 or static_cast<size_t>(n) >= server->workspaces.size())
        return {"Invalid Workspace number"};

    server->seat.focus(server, &server->workspaces[n]);
    return { "Changed to workspace: "s + std::to_string(n) };
}

inline CommandResult workspace_move(Server* server, int n)
{
    using namespace std::string_literals;

    if(n < 0 or static_cast<size_t>(n) >= server->workspaces.size())
        return {"Invalid Workspace number"};

    View* view = server->seat.get_focused_view();
    server->workspaces[view->workspace_id].remove_view(view);
    server->workspaces[n].add_view(view, server->workspaces[n].tiles.back().view);

    return { "Moved focused window to workspace "s + std::to_string(n) };
}

};

#endif // __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
