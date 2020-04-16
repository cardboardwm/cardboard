extern "C" {
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
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

    wlr_seat_pointer_notify_button(cursor->seat->wlr_seat, event->time_msec, event->button, event->state);
    double sx, sy;
    struct wlr_surface* surface;
    View* view = server->get_surface_under_cursor(cursor->wlr_cursor->x, cursor->wlr_cursor->y, surface, sx, sy);
    if (event->state == WLR_BUTTON_RELEASED) {
        // end grabbing
        cursor->seat->end_interactive();
    } else if (view != cursor->seat->get_focused_view() && view != nullptr) {
        cursor->seat->focus_view(server, view);
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
