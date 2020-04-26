extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
}

#include "Cursor.h"
#include "Server.h"

void init_cursor(Server* server, Seat* seat, SeatCursor* cursor)
{
    cursor->seat = seat;

    cursor->wlr_cursor = wlr_cursor_create();
    cursor->wlr_cursor->data = cursor;
    wlr_cursor_attach_output_layout(cursor->wlr_cursor, server->output_layout);

    cursor->wlr_cursor_manager = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(cursor->wlr_cursor_manager, 1);

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &cursor->wlr_cursor->events.motion, cursor_motion_handler },
        { &cursor->wlr_cursor->events.motion_absolute, cursor_motion_absolute_handler },
        { &cursor->wlr_cursor->events.button, cursor_button_handler },
        { &cursor->wlr_cursor->events.axis, cursor_axis_handler },
        { &cursor->wlr_cursor->events.frame, cursor_frame_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(to_add_listener.signal,
                                       Listener { to_add_listener.notify, server, cursor });
    }
}

void cursor_motion_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_motion*>(data);

    wlr_cursor_move(cursor->wlr_cursor, event->device, event->delta_x, event->delta_y);
    cursor->seat->process_cursor_motion(server, event->time_msec);
}

void cursor_motion_absolute_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(cursor->wlr_cursor, event->device, event->x, event->y);
    cursor->seat->process_cursor_motion(server, event->time_msec);
}

void cursor_button_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_button*>(data);

    if (event->state == WLR_BUTTON_RELEASED) {
        wlr_seat_pointer_notify_button(cursor->seat->wlr_seat, event->time_msec, event->button, event->state);
        // end grabbing
        cursor->seat->end_interactive();
        return;
    }

    double sx, sy;
    struct wlr_surface* surface;
    View* view = server->get_surface_under_cursor(cursor->wlr_cursor->x, cursor->wlr_cursor->y, surface, sx, sy);
    if (!view) {
        return;
    }
    if (cursor->seat->is_mod_pressed()) {
        if (event->button == BTN_LEFT) {
            cursor->seat->begin_move(server, view);
        } else if (event->button == BTN_RIGHT) {
            uint32_t edge = 0;
            edge |= cursor->wlr_cursor->x > view->x + view->geometry.x + view->geometry.width / 2 ? WLR_EDGE_RIGHT : WLR_EDGE_LEFT;
            edge |= cursor->wlr_cursor->y > view->y + view->geometry.y + view->geometry.height / 2 ? WLR_EDGE_BOTTOM : WLR_EDGE_TOP;
            cursor->seat->begin_resize(server, view, edge);
        }
    } else {
        wlr_seat_pointer_notify_button(cursor->seat->wlr_seat, event->time_msec, event->button, event->state);
        if (view != cursor->seat->get_focused_view()) {
            cursor->seat->focus_view(server, view);
        }
    }
}

void cursor_axis_handler(struct wl_listener* listener, void* data)
{
    auto* cursor = get_listener_data<SeatCursor*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_axis*>(data);

    wlr_seat_pointer_notify_axis(cursor->seat->wlr_seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}

void cursor_frame_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* cursor = get_listener_data<SeatCursor*>(listener);

    wlr_seat_pointer_notify_frame(cursor->seat->wlr_seat);
}
