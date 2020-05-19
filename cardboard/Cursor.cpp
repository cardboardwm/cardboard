extern "C" {
#include <linux/input-event-codes.h>
#include <wayland-server.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
}

#include "Cursor.h"
#include "Server.h"

void SeatCursor::register_image_surface(Server* server, struct wlr_surface* surface)
{
    image_surface_destroy_listener = image_surface_destroy_listener.and_then<struct wl_listener>([server](auto& listener) {
        server->listeners.remove_listener(&listener);
        return NullRef<struct wl_listener>;
    });
    if (surface) {
        image_surface_destroy_listener = OptionalRef(
            server->listeners.add_listener(&surface->events.destroy,
                                           Listener { SeatCursor::image_surface_destroy_handler, server, this }));
    }
}

void SeatCursor::set_image(Server* server, const char* image)
{
    if (!(seat->wlr_seat->capabilities & WL_SEAT_CAPABILITY_POINTER)) {
        return;
    }

    register_image_surface(server, nullptr);
    if (!image) {
        wlr_cursor_set_image(wlr_cursor, nullptr, 0, 0, 0, 0, 0, 0);
    } else {
        wlr_xcursor_manager_set_cursor_image(wlr_xcursor_manager, image, wlr_cursor);
    }
}

void SeatCursor::set_image_surface(Server* server, struct wlr_surface* surface, int32_t hotspot_x, int32_t hotspot_y)
{
    if (!(seat->wlr_seat->capabilities & WL_SEAT_CAPABILITY_POINTER)) {
        return;
    }

    register_image_surface(server, surface);
    wlr_cursor_set_surface(wlr_cursor, surface, hotspot_x, hotspot_y);
}

void SeatCursor::rebase(Server* server, uint32_t time)
{
    // time is an optional argument defaulting to 0
    if (time == 0) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        time = now.tv_nsec / 1000;
    }

    double sx, sy;
    struct wlr_surface* surface = nullptr;
    server->get_surface_under_cursor(wlr_cursor->x, wlr_cursor->y, surface, sx, sy);
    if (!surface) {
        // set the cursor to default
        set_image(server, "left_ptr");
    }
    if (surface && seat->is_input_allowed(surface)) {
        bool focus_changed = seat->wlr_seat->pointer_state.focused_surface != surface;
        // Gives pointer focus when the cursor enters the surface
        wlr_seat_pointer_notify_enter(seat->wlr_seat, surface, sx, sy);
        if (!focus_changed) {
            // the enter event contains coordinates, so we notify on motion only
            // if the focus did not change
            wlr_seat_pointer_notify_motion(seat->wlr_seat, time, sx, sy);
        }
    } else {
        // Clear focus so pointer events are not sent to the last entered surface
        wlr_seat_pointer_clear_focus(seat->wlr_seat);
    }
}
void SeatCursor::warp(Server* server, int lx, int ly) const
{
    wlr_cursor_warp(wlr_cursor, nullptr, lx, ly);
    seat->process_cursor_motion(server);
}

void init_cursor(Server* server, Seat* seat, SeatCursor* cursor)
{
    cursor->seat = seat;

    cursor->wlr_cursor = wlr_cursor_create();
    cursor->wlr_cursor->data = cursor;
    wlr_cursor_attach_output_layout(cursor->wlr_cursor, server->output_layout);

    cursor->wlr_xcursor_manager = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(cursor->wlr_xcursor_manager, 1);

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &cursor->wlr_cursor->events.motion, SeatCursor::motion_handler },
        { &cursor->wlr_cursor->events.motion_absolute, SeatCursor::motion_absolute_handler },
        { &cursor->wlr_cursor->events.button, SeatCursor::button_handler },
        { &cursor->wlr_cursor->events.axis, SeatCursor::axis_handler },
        { &cursor->wlr_cursor->events.frame, SeatCursor::frame_handler },

        { &cursor->wlr_cursor->events.swipe_begin, SeatCursor::swipe_begin_handler },
        { &cursor->wlr_cursor->events.swipe_update, SeatCursor::swipe_update_handler },
        { &cursor->wlr_cursor->events.swipe_end, SeatCursor::swipe_end_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(to_add_listener.signal,
                                       Listener { to_add_listener.notify, server, cursor });
    }
}

void SeatCursor::motion_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_motion*>(data);

    // in case the user was doing a three finger swipe and lifted two fingers.
    cursor->seat->end_touchpad_swipe(server);

    wlr_cursor_move(cursor->wlr_cursor, event->device, event->delta_x, event->delta_y);
    cursor->seat->process_cursor_motion(server, event->time_msec);
}

void SeatCursor::motion_absolute_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(cursor->wlr_cursor, event->device, event->x, event->y);
    cursor->seat->process_cursor_motion(server, event->time_msec);
}

void SeatCursor::button_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_button*>(data);

    if (event->state == WLR_BUTTON_RELEASED) {
        wlr_seat_pointer_notify_button(cursor->seat->wlr_seat, event->time_msec, event->button, event->state);
        // end grabbing
        cursor->seat->end_interactive(server);
        return;
    }

    double sx, sy;
    struct wlr_surface* surface;
    View* view = server->get_surface_under_cursor(cursor->wlr_cursor->x, cursor->wlr_cursor->y, surface, sx, sy);
    if (!view) {
        wlr_seat_pointer_notify_button(cursor->seat->wlr_seat, event->time_msec, event->button, event->state);
        return;
    }
    if (cursor->seat->is_mod_pressed(server->config.mouse_mods)) {
        if (event->button == BTN_LEFT) {
            cursor->set_image(server, "grab");
            cursor->seat->begin_move(server, view);
            wlr_seat_pointer_clear_focus(cursor->seat->wlr_seat); // must be _after_ begin_move
        } else if (event->button == BTN_RIGHT) {
            uint32_t edge = 0;
            edge |= cursor->wlr_cursor->x > view->x + view->geometry.x + view->geometry.width / 2 ? WLR_EDGE_RIGHT : WLR_EDGE_LEFT;
            edge |= cursor->wlr_cursor->y > view->y + view->geometry.y + view->geometry.height / 2 ? WLR_EDGE_BOTTOM : WLR_EDGE_TOP;
            const char* image = NULL;
            if (edge == (WLR_EDGE_LEFT | WLR_EDGE_TOP)) {
                image = "nw-resize";
            } else if (edge == (WLR_EDGE_TOP | WLR_EDGE_RIGHT)) {
                image = "ne-resize";
            } else if (edge == (WLR_EDGE_RIGHT | WLR_EDGE_BOTTOM)) {
                image = "se-resize";
            } else if (edge == (WLR_EDGE_BOTTOM | WLR_EDGE_LEFT)) {
                image = "sw-resize";
            }

            cursor->set_image(server, image);
            cursor->seat->begin_resize(server, view, edge);
            wlr_seat_pointer_clear_focus(cursor->seat->wlr_seat); // must be _after_ begin_resize
        }
    } else {
        wlr_seat_pointer_notify_button(cursor->seat->wlr_seat, event->time_msec, event->button, event->state);
        if (view != cursor->seat->get_focused_view()) {
            cursor->seat->focus_view(server, view);
        }
    }
}

void SeatCursor::axis_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_axis*>(data);

    // in case the user was doing a three finger swipe and lifted a finger
    cursor->seat->end_touchpad_swipe(server);

    wlr_seat_pointer_notify_axis(cursor->seat->wlr_seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}

void SeatCursor::frame_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* cursor = get_listener_data<SeatCursor*>(listener);

    wlr_seat_pointer_notify_frame(cursor->seat->wlr_seat);
}

void SeatCursor::image_surface_destroy_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);

    cursor->set_image(server, nullptr);
    cursor->rebase(server);
}

void SeatCursor::swipe_begin_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_swipe_begin*>(data);

    cursor->seat->process_swipe_begin(server, event->fingers);
}

void SeatCursor::swipe_update_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_swipe_update*>(data);

    cursor->seat->process_swipe_update(server, event->fingers, event->dx, event->dy);
}

void SeatCursor::swipe_end_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);

    cursor->seat->process_swipe_end(server);
}
