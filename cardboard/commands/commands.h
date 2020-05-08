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
#include "../ViewManager.h"

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

    if (n < 0 or static_cast<size_t>(n) >= server->workspaces.size())
        return { "Invalid Workspace number" };

    server->seat.focus(server, &server->workspaces[n]);
    return { "Changed to workspace: "s + std::to_string(n) };
}

inline CommandResult workspace_move(Server* server, int n)
{
    using namespace std::string_literals;

    if (n < 0 or static_cast<size_t>(n) >= server->workspaces.size())
        return { "Invalid Workspace number" };

    View* view = server->seat.get_focused_view();
    change_view_workspace(server, view, &server->workspaces[n]);

    return { "Moved focused window to workspace "s + std::to_string(n) };
}

inline CommandResult focus_cycle(Server* server)
{
    auto& focus_stack = server->seat.focus_stack;
    auto current_workspace = server->seat.get_focused_workspace(server);

    if(!current_workspace) {
        return {""};
    }

    if(auto it = std::find_if(
            ++focus_stack.begin(),
            focus_stack.end(),
            [current_workspace](View* view) {
                return view->workspace_id == current_workspace.unwrap().index;
            }); it != focus_stack.end())
    {
        View* view = *it;
        server->seat.focus_view(server, view);

        auto previous_view_it = ++server->seat.focus_stack.begin();
        auto previous_view = *previous_view_it;
        server->seat.focus_stack.erase(previous_view_it);
        server->seat.focus_stack.push_back(previous_view);
    }

    return {""};
}

inline CommandResult toggle_floating(Server* server)
{
    View* view = server->seat.get_focused_view();

    bool currently_floating =
        server->workspaces[view->workspace_id].find_floating(view) != server->workspaces[view->workspace_id].floating_views.end();

    if (!currently_floating) {
        view->set_fullscreen(false);
    }
    server->workspaces[view->workspace_id].remove_view(view);
    server->workspaces[view->workspace_id].add_view(view, server->workspaces[view->workspace_id].tiles.back().view, !currently_floating);

    return {""};

}

inline CommandResult move(Server* server, int dx, int dy)
{
    View* view = server->seat.get_focused_view();
    Workspace& workspace = server->workspaces[view->workspace_id];

    if(auto it = workspace.find_tile(view); it != workspace.tiles.end()) {
        auto other = it;

        std::advance(other, dx / abs(dx));

        if((it == workspace.tiles.begin() && dx < 0) || other == workspace.tiles.end()) {
            return {""};
        }

        std::swap(*other, *it);
        workspace.arrange_tiles();
    } else {
        reconfigure_view_position(server, view, view->x + dx, view->y + dy);
    }

    return {""};
}

inline CommandResult resize(Server* server, int width, int height)
{
    View* view = server->seat.get_focused_view();
    reconfigure_view_size(server, view, width, height);
    return {""};
}

};

#endif // __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
