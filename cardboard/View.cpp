// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

#include "Server.h"
#include "View.h"

OptionalRef<Output> View::get_views_output(Server& server)
{
    if (workspace_id < 0) {
        return NullRef<Output>;
    }
    return server.output_manager->get_view_workspace(*this).output;
}

void View::change_output(OptionalRef<Output> old_output, OptionalRef<Output> new_output)
{
    if (old_output && old_output != new_output) {
        wlr_surface_send_leave(get_surface(), old_output.unwrap().wlr_output);
    }

    if (new_output) {
        wlr_surface_send_enter(get_surface(), new_output.unwrap().wlr_output);
    }
}

void View::save_state(ExpansionSavedState to_save)
{
    assert(!saved_state.has_value() && expansion_state == ExpansionState::NORMAL);

    saved_state = to_save;
    wlr_log(WLR_DEBUG, "saved size (%4d, %4d)", saved_state->width, saved_state->height);
}

void View::recover()
{
    if (expansion_state == ExpansionState::RECOVERING) {
        expansion_state = ExpansionState::NORMAL;
    }
}

void View::cycle_width(int screen_width)
{
    if (screen_width == 0) {
        // shouldn't happen, but better safe than sorry
        return;
    }
    static constexpr double golden_ratio = 0x1.3c6ef372fe94fp-1; // 1 / phi
    constexpr std::array<double, 3> ratios = { 1 - golden_ratio, 0.5, golden_ratio };
    double current_ratio = static_cast<double>(geometry.width) / static_cast<double>(screen_width);

    decltype(ratios)::size_type i = 0;
    while (i < ratios.size() && ratios[i] <= current_ratio) {
        i++;
    }

    if (fabs(current_ratio - ratios[i]) < 0.01) {
        i = (i + 1) % ratios.size();
    }

    if (i == ratios.size()) {
        i = 0;
    }
    resize(static_cast<int>(screen_width * ratios[i]), geometry.height);
}

void View::move(OutputManager& output_manager, int x_, int y_)
{
    x = x_;
    y = y_;
    output_manager.set_dirty();
}

bool View::is_mapped_and_normal()
{
    return mapped && expansion_state == View::ExpansionState::NORMAL;
}

void View::resize(int width, int height)
{
    target_width = width;
    target_height = height;
}

void create_view(Server& server, NotNullPointer<View> view_)
{
    server.surface_manager.views.emplace_back(view_);
    auto* view = server.surface_manager.views.back().get();

    view->prepare(server);
}
