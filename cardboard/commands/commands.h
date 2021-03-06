// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
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

inline CommandResult config_gap(Server* server, int gap)
{
    server->config.gap = gap;
    for (auto& workspace : server->output_manager->workspaces) {
        workspace.arrange_workspace(*(server->output_manager), true);
    }
    return { "" };
}

inline CommandResult config_focus_color(Server* server, float r, float g, float b, float a)
{
    server->config.focus_color = { r, g, b, a };
    return { "" };
}

inline CommandResult focus(Server* server, command_arguments::focus::Direction direction)
{
    using namespace std::string_literals;

    auto focused_view_ = server->seat.get_focused_view();
    if (!focused_view_) {
        return { "No focused view to use as reference"s };
    }
    auto& focused_view = focused_view_.unwrap();
    auto& workspace = server->output_manager->get_view_workspace(focused_view);

    auto column_it = workspace.find_column(&focused_view);
    if (column_it == workspace.columns.end()) {
        return { "Focused view is floating"s };
    }

    if (direction == command_arguments::focus::Direction::Left || direction == command_arguments::focus::Direction::Right) {
        int offset = direction == command_arguments::focus::Direction::Left ? -1 : +1;
        if (int index = std::distance(workspace.columns.begin(), column_it) + offset;
            index < 0 || index >= static_cast<int>(workspace.columns.size())) {
            // out of bounds
            return { "" };
        }

        std::advance(column_it, offset);
        server->seat.focus_column(*server, *column_it);
    } else {
        if (direction == command_arguments::focus::Direction::Up) {
            auto tile_it = std::find_if(column_it->tiles.rbegin(), column_it->tiles.rend(), [&focused_view](const auto& tile) {
                return tile.view == &focused_view;
            });
            if (tile_it == column_it->tiles.rend()) {
                return { "" };
            }

            for (tile_it++; tile_it != column_it->tiles.rend(); tile_it++) {
                if (tile_it->view->is_mapped_and_normal()) {
                    server->seat.focus_view(*server, OptionalRef(tile_it->view));
                    break;
                }
            }
        } else {
            auto tile_it = std::find_if(column_it->tiles.begin(), column_it->tiles.end(), [&focused_view](const auto& tile) {
                return tile.view == &focused_view;
            });
            if (tile_it == column_it->tiles.end()) {
                return { "" };
            }

            for (tile_it++; tile_it != column_it->tiles.end(); tile_it++) {
                if (tile_it->view->is_mapped_and_normal()) {
                    server->seat.focus_view(*server, OptionalRef(tile_it->view));
                    break;
                }
            }
        }
    }

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

    if (n < 0 or static_cast<size_t>(n) >= server->output_manager->workspaces.size())
        return { "Invalid Workspace number" };

    server->seat.focus(*server, server->output_manager->workspaces[n]);
    server->output_manager->workspaces[n].arrange_workspace(*(server->output_manager));
    return { "Changed to workspace: "s + std::to_string(n) };
}

inline CommandResult workspace_move(Server* server, int n)
{
    using namespace std::string_literals;

    if (n < 0 or static_cast<size_t>(n) >= server->output_manager->workspaces.size())
        return { "Invalid Workspace number" };

    auto view = server->seat.get_focused_view();
    if (!view) {
        return { "No view to move in current workspace"s };
    }
    change_view_workspace(*server, view.unwrap(), server->output_manager->workspaces[n]);
    server->output_manager->workspaces[n].arrange_workspace(*(server->output_manager));

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
    auto& ws = server->output_manager->get_view_workspace(view);

    bool currently_floating = server->output_manager->workspaces[view.workspace_id].find_floating(&view) != server->output_manager->workspaces[view.workspace_id].floating_views.end();

    auto prev_size = view.previous_size;

    if (ws.fullscreen_view.raw_pointer() == &view) {
        std::swap(view.saved_state->width, view.previous_size.first);
        std::swap(view.saved_state->height, view.previous_size.second);
    } else {
        view.previous_size = { view.geometry.width, view.geometry.height };
        view.resize(prev_size.first, prev_size.second);
    }

    ws.remove_view(*(server->output_manager), view, true);
    ws.add_view(*(server->output_manager), view, ws.columns.empty() ? nullptr : ws.columns.back().tiles.back().view.get(), !currently_floating, true);

    // automatically scroll workspace to the previously focused view from this workspace so the newly floating view does not leave a hole
    if (!currently_floating) {
        for (auto prev_focused_view_ptr : server->seat.focus_stack) {
            if (prev_focused_view_ptr == &view) {
                continue;
            }
            auto& this_ws = server->output_manager->get_view_workspace(*prev_focused_view_ptr);
            if (&this_ws == &ws) {
                this_ws.fit_view_on_screen(*server->output_manager, *prev_focused_view_ptr, true);
                break;
            }
        }
    } else {
        ws.fit_view_on_screen(*server->output_manager, view);
    }

    return { "" };
}

inline CommandResult move(Server* server, int dx, int dy)
{
    auto view_ = server->seat.get_focused_view();
    if (!view_) {
        return { "" };
    }
    auto& view = view_.unwrap();
    Workspace& workspace = server->output_manager->workspaces[view.workspace_id];

    if (auto it = workspace.find_column(&view); it != workspace.columns.end()) {
        auto other = it;
        auto current_column = it;

        if (dx != 0) {
            std::advance(other, dx / abs(dx));

            if (other != workspace.columns.end() && other != std::prev(workspace.columns.begin())) {
                std::swap(*other, *it);
                current_column = other;
            }
        }

        if (dy != 0 && current_column->tiles.size() > 1) {
            auto focused_tile = std::find_if(
                current_column->tiles.begin(),
                current_column->tiles.end(),
                [&view](const auto& x) {
                    return &view == x.view.get();
                });

            auto index = std::distance(current_column->tiles.begin(), focused_tile);
            index = (index - dy / std::abs(dy) + current_column->tiles.size()) % current_column->tiles.size();

            auto other_tile = current_column->tiles.begin();
            std::advance(other_tile, index);
            std::swap(
                *focused_tile,
                *other_tile);
        }

        workspace.arrange_workspace(*(server->output_manager), true);
        workspace.fit_view_on_screen(*(server->output_manager), *current_column->tiles.begin()->view);
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

inline CommandResult insert_into_column(Server* server)
{
    using namespace std::string_literals;

    auto view = server->seat.get_focused_view();
    if (!view) {
        return { "No view to move in current workspace"s };
    }

    auto& workspace = server->output_manager->get_view_workspace(view.unwrap());
    auto column_it = workspace.find_column(&view.unwrap());
    if (&*column_it == &workspace.columns.back()) {
        // nothing to do
        return { "Nothing to do"s };
    }
    auto next_column_it = std::next(column_it);

    for (auto& tile : next_column_it->mapped_and_normal_tiles()) {
        workspace.insert_into_column(*(server->output_manager), *tile.view, *column_it);
        return { "" };
    }

    return { "Nothing to do"s };
}

inline CommandResult pop_from_column(Server* server)
{
    using namespace std::string_literals;

    auto view = server->seat.get_focused_view();
    if (!view) {
        return { "No view to move in current workspace"s };
    }

    if (view.unwrap().expansion_state != View::ExpansionState::NORMAL) {
        return { "View must not be fullscreened"s };
    }

    auto& workspace = server->output_manager->get_view_workspace(view.unwrap());
    auto column_it = workspace.find_column(&view.unwrap());
    if (column_it == workspace.columns.end()) {
        return { "View is floating"s };
    }

    workspace.pop_from_column(*(server->output_manager), *column_it);
    workspace.fit_view_on_screen(*(server->output_manager), view.unwrap());
    return { "" };
}

inline CommandResult cycle_width(Server* server)
{
    auto focused_view_ = server->seat.get_focused_view();
    if (!focused_view_) {
        return { "" };
    }
    auto& focused_view = focused_view_.unwrap();

    auto& ws = server->output_manager->get_view_workspace(focused_view);
    ws.output.and_then([server, &focused_view, &ws](const auto& output) {
        const struct wlr_box* output_box = server->output_manager->get_output_box(output);
        focused_view.cycle_width(output_box->width);
        // trick to make arrange_workspace work correctly
        focused_view.geometry.width = focused_view.target_width;
        focused_view.geometry.height = focused_view.target_height;

        ws.fit_view_on_screen(*server->output_manager, focused_view, true);
    });
    return { "" };
}

};

#endif // CARDBOARD_COMMANDS_COMMANDS_H_INCLUDED
