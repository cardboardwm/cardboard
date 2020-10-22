// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
}

#include "Cursor.h"
#include "Server.h"

static void image_surface_destroy_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);

    cursor_set_image(*server, server->seat, *cursor, nullptr);
    cursor_rebase(*server, server->seat, *cursor);
}

/**
  * \brief Registers a destroy handler (SeatCursor::image_surface_destroy_handler) on \a surface
  * to correctly unset the surface as the cursor after it gets destroyed.
  */
static void register_image_surface(Server& server, SeatCursor& cursor, struct wlr_surface* surface)
{
    cursor.image_surface_destroy_listener = cursor.image_surface_destroy_listener.and_then<struct wl_listener>([&server](auto& listener) {
        server.listeners.remove_listener(&listener);
        return NullRef<struct wl_listener>;
    });
    if (surface) {
        cursor.image_surface_destroy_listener = OptionalRef(
            server.listeners.add_listener(&surface->events.destroy,
                                          Listener { image_surface_destroy_handler, &server, &cursor }));
    }
}

void init_cursor(OutputManager& output_manager, SeatCursor& cursor)
{
    cursor.wlr_cursor = wlr_cursor_create();
    cursor.wlr_cursor->data = &cursor;
    wlr_cursor_attach_output_layout(cursor.wlr_cursor, output_manager.output_layout);

    cursor.wlr_xcursor_manager = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(cursor.wlr_xcursor_manager, 1);
}

void cursor_set_image(Server& server, Seat& seat, SeatCursor& cursor, const char* image)
{
    if (!(seat.wlr_seat->capabilities & WL_SEAT_CAPABILITY_POINTER)) {
        return;
    }

    register_image_surface(server, cursor, nullptr);
    if (!image) {
        wlr_cursor_set_image(cursor.wlr_cursor, nullptr, 0, 0, 0, 0, 0, 0);
    } else {
        wlr_xcursor_manager_set_cursor_image(cursor.wlr_xcursor_manager, image, cursor.wlr_cursor);
    }
}

void cursor_set_image_surface(Server& server, Seat& seat, SeatCursor& cursor, struct wlr_surface* surface, int32_t hotspot_x, int32_t hotspot_y)
{
    if (!(seat.wlr_seat->capabilities & WL_SEAT_CAPABILITY_POINTER)) {
        return;
    }

    register_image_surface(server, cursor, surface);
    wlr_cursor_set_surface(cursor.wlr_cursor, surface, hotspot_x, hotspot_y);
}

void cursor_rebase(Server& server, Seat& seat, SeatCursor& cursor, uint32_t time)
{
    // time is an optional argument defaulting to 0
    if (time == 0) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        time = now.tv_nsec / 1000;
    }

    double sx, sy;
    struct wlr_surface* surface = nullptr;
    server.surface_manager.get_surface_under_cursor(*(server.output_manager), cursor.wlr_cursor->x, cursor.wlr_cursor->y, surface, sx, sy);
    if (!surface) {
        // set the cursor to default
        cursor_set_image(server, seat, cursor, "left_ptr");
    }
    if (surface && seat.is_input_allowed(surface)) {
        bool focus_changed = seat.wlr_seat->pointer_state.focused_surface != surface;
        // Gives pointer focus when the cursor enters the surface
        wlr_seat_pointer_notify_enter(seat.wlr_seat, surface, sx, sy);
        if (!focus_changed) {
            // the enter event contains coordinates, so we notify on motion only
            // if the focus did not change
            wlr_seat_pointer_notify_motion(seat.wlr_seat, time, sx, sy);
        }
    } else {
        // Clear focus so pointer events are not sent to the last entered surface
        wlr_seat_pointer_clear_focus(seat.wlr_seat);
    }
}
void cursor_warp(Server& server, Seat& seat, SeatCursor& cursor, int lx, int ly)
{
    wlr_cursor_warp(cursor.wlr_cursor, nullptr, lx, ly);
    seat.process_cursor_motion(server);
}
