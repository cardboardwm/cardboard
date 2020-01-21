#ifndef __CARDBOARD_EVENT_LISTENERS_H_
#define __CARDBOARD_EVENT_LISTENERS_H_

#include <wayland-server.h>

struct Server;

struct EventListeners {
    Server* server;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener new_xdg_surface;
    struct wl_listener new_input;
    struct wl_listener request_cursor;
    struct wl_listener new_output;

    static void new_output_handler(struct wl_listener* listener, void* data);
    static void new_xdg_surface_handler(struct wl_listener* listener, void* data);
    static void cursor_motion_handler(struct wl_listener* listener, void* data);
    static void cursor_motion_absolute_handler(struct wl_listener* listener, void* data);
    static void cursor_button_handler(struct wl_listener* listener, void* data);
    static void cursor_axis_handler(struct wl_listener* listener, void* data);
    static void cursor_frame_handler(struct wl_listener* listener, void* data);
    static void new_input_handler(struct wl_listener* listener, void* data);
    static void seat_request_cursor_handler(struct wl_listener* listener, void* data);

    EventListeners() {}
    EventListeners(const EventListeners&) = delete;
};

#endif // __CARDBOARD_EVENT_LISTENERS_H_
