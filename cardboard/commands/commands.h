#ifndef CARDBOARD_COMMANDS_COMMANDS_H_INCLUDED
#define CARDBOARD_COMMANDS_COMMANDS_H_INCLUDED

#include <csignal>
#include <cstdint>
#include <locale>
#include <string>
#include <string_view>

#include "../Command.h"
#include "../IPC.h"
#include "../Server.h"
#include "../Spawn.h"
#include "../ViewOperations.h"

extern char** environ;

namespace commands {

inline CommandResult config_mouse_mod(Server* server, uint32_t modifiers)
{
    server->config.mouse_mods = modifiers;
    return { "" };
}

inline CommandResult focus(Server* server, int focus_direction)
{
    server->seat.focus_by_offset(*server, focus_direction);
    return { "" };
}

inline CommandResult quit(Server* server, int code)
{
    server->teardown(code);
    return { "" };
}

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
    server->seat.get_focused_view().and_then([](auto& fview) {
        fview.close();
    });

    return { "" };
}

inline CommandResult workspace_switch(Server* server, int n)
{
    using namespace std::string_literals;

    if (n < 0 or static_cast<size_t>(n) >= server->output_manager.workspaces.size())
        return { "Invalid Workspace number" };

    server->seat.focus(*server, server->output_manager.workspaces[n]);
    server->output_manager.workspaces[n].arrange_workspace(server->output_manager);
    return { "Changed to workspace: "s + std::to_string(n) };
}

inline CommandResult workspace_move(Server* server, int n)
{
    using namespace std::string_literals;

    if (n < 0 or static_cast<size_t>(n) >= server->output_manager.workspaces.size())
        return { "Invalid Workspace number" };

    auto view = server->seat.get_focused_view();
    if (!view) {
        return { "No view to move in current workspace"s };
    }
    change_view_workspace(*server, view.unwrap(), server->output_manager.workspaces[n]);
    server->output_manager.workspaces[n].arrange_workspace(server->output_manager);

    return { "Moved focused window to workspace "s + std::to_string(n) };
}

inline CommandResult focus_cycle(Server* server)
{
    auto& focus_stack = server->seat.focus_stack;
    auto current_workspace = server->seat.get_focused_workspace(*server);

    if (!current_workspace) {
        return { "" };
    }

    if (auto it = std::find_if(
            std::next(focus_stack.begin()),
            focus_stack.end(),
            [current_workspace](View* view) {
                return view->workspace_id == current_workspace.unwrap().index;
            });
        it != focus_stack.end()) {
        View* view = *it;
        server->seat.focus_view(*server, OptionalRef(view));

        auto previous_view_it = std::next(server->seat.focus_stack.begin());
        auto previous_view = *previous_view_it;
        server->seat.focus_stack.erase(previous_view_it);
        server->seat.focus_stack.push_back(previous_view);
    }

    return { "" };
}

inline CommandResult toggle_floating(Server* server)
{
    auto view_ = server->seat.get_focused_view();
    if (!view_) {
        return { "" };
    }
    auto& view = view_.unwrap();
    auto& ws = server->output_manager.get_view_workspace(view);

    bool currently_floating = server->output_manager.workspaces[view.workspace_id].find_floating(&view) != server->output_manager.workspaces[view.workspace_id].floating_views.end();

    auto prev_size = view.previous_size;
    view.previous_size = { view.geometry.width, view.geometry.height };

    if (ws.fullscreen_view.raw_pointer() == &view) {
        view.saved_state->width = prev_size.first;
        view.saved_state->height = prev_size.second;
    } else {
        view.resize(prev_size.first, prev_size.second);
    }

    ws.remove_view(server->output_manager, view, true);
    ws.add_view(server->output_manager, view, ws.columns.empty() ? nullptr : ws.columns.back().tiles.back().view.get(), !currently_floating, true);

    return { "" };
}

inline CommandResult move(Server* server, int dx, int dy)
{
    auto view_ = server->seat.get_focused_view();
    if (!view_) {
        return { "" };
    }
    auto& view = view_.unwrap();
    Workspace& workspace = server->output_manager.workspaces[view.workspace_id];

    if (auto it = workspace.find_column(&view); it != workspace.columns.end()) {
        auto other = it;

        std::advance(other, dx / abs(dx));

        if ((it == workspace.columns.begin() && dx < 0) || other == workspace.columns.end()) {
            return { "" };
        }

        std::swap(*other, *it);
        workspace.arrange_workspace(server->output_manager);
    } else {
        reconfigure_view_position(*server, view, view.x + dx, view.y + dy);
    }

    return { "" };
}

inline CommandResult resize(Server* server, int width, int height)
{
    server->seat.get_focused_view().and_then([server, width, height](auto& view) {
        reconfigure_view_size(*server, view, width, height);
    });

    return { "" };
}

inline CommandResult cycle_width(Server* server)
{
    auto focused_view_ = server->seat.get_focused_view();
    if (!focused_view_) {
        return { "" };
    }
    auto& focused_view = focused_view_.unwrap();

    server->output_manager.get_view_workspace(focused_view).output.and_then([server, &focused_view](const auto& output) {
        const struct wlr_box* output_box = server->output_manager.get_output_box(output);
        focused_view.cycle_width(output_box->width);
    });
    return { "" };
}

};

#endif // CARDBOARD_COMMANDS_COMMANDS_H_INCLUDED
