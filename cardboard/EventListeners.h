#ifndef __CARDBOARD_EVENT_LISTENERS_H_
#define __CARDBOARD_EVENT_LISTENERS_H_

#include <wayland-server.h>

void new_output_handler(struct wl_listener* listener, void* data);
void new_xdg_surface_handler(struct wl_listener* listener, void* data);
void cursor_motion_handler(struct wl_listener* listener, void* data);
void cursor_motion_absolute_handler(struct wl_listener* listener, void* data);
void cursor_button_handler(struct wl_listener* listener, void* data);
void cursor_axis_handler(struct wl_listener* listener, void* data);
void cursor_frame_handler(struct wl_listener* listener, void* data);
void new_input_handler(struct wl_listener* listener, void* data);
void seat_request_cursor_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_EVENT_LISTENERS_H_
