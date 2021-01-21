// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_OUTPUT_H_INCLUDED
#define CARDBOARD_OUTPUT_H_INCLUDED

extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_damage.h>
}

#include <array>

#include "Layers.h"
#include "Server.h"

/**
 * \file
 * \brief This file contains listeners and helper functions for simple output operations.
 *
 * Outputs are displays.
 */

struct Output {
    struct wlr_output* wlr_output;
    struct wlr_output_damage* wlr_output_damage;
    struct wlr_box usable_area;

    /// Time of last presentation. Use it to calculate the delta time.
    struct timespec last_present;

    /// Executed for each frame render per output.
    static void frame_handler(struct wl_listener* listener, void* data);
    /// Executed as soon as the first pixel is put on the screen;
    static void present_handler(struct wl_listener* listener, void* data);
    /// Executed when the output is detached.
    static void destroy_handler(struct wl_listener* listener, void* data);
    /// Executed when the output changes fields (transform, scale etc).
    static void commit_handler(struct wl_listener* listener, void* data);
    /// Executed when the mode changes (resolution, framerate etc)
    static void mode_handler(struct wl_listener* listener, void* data);
};

/// Registers event listeners and does bookkeeping for a newly added output.
void register_output(Server& server, Output&& output);

#endif // CARDBOARD_OUTPUT_H_INCLUDED
