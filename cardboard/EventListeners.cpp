#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_cursor.h>
#include <wlr_cpp/types/wlr_output.h>
#include <wlr_cpp/types/wlr_output_layout.h>

#include "EventListeners.h"
#include "Server.h"

void EventListeners::new_output_handler(struct wl_listener* listener, void* data)
{
    EventListeners* listeners = wl_container_of(listener, listeners, new_output);
    Server* server = listeners->server;
    auto* wlr_output = static_cast<struct wlr_output*>(data);

    // pick the monitor's preferred mode
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            return;
        }
    }

    server->outputs.emplace_back(server, wlr_output);

    // add output to the layout. add_auto arranges outputs left-to-right
    // in the order they appear.
    wlr_output_layout_add_auto(server->output_layout, wlr_output);

    // expose the output to the clients
    wlr_output_create_global(wlr_output);
}
void EventListeners::new_xdg_surface_handler(struct wl_listener* listener, void* data)
{
    EventListeners* listeners = wl_container_of(listener, listeners, new_xdg_surface);
    Server* server = listeners->server;
    auto* xdg_surface = static_cast<struct wlr_xdg_surface*>(data);

    // ignore popups TODO: don't ignore them
    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    auto this_it = server->views.emplace(server->views.begin(), server, xdg_surface);
    this_it->link = this_it;
}
void EventListeners::cursor_motion_handler(struct wl_listener* listener, void* data)
{
    EventListeners* listeners = wl_container_of(listener, listeners, cursor_motion);
    Server* server = listeners->server;
    auto* event = static_cast<struct wlr_event_pointer_motion*>(data);
    wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
    server->process_cursor_motion(event->time_msec);
}
void EventListeners::cursor_motion_absolute_handler(struct wl_listener* listener, void* data)
{
    EventListeners* listeners = wl_container_of(listener, listeners, cursor_motion_absolute);
    Server* server = listeners->server;
    auto* event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
    wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
    server->process_cursor_motion(event->time_msec);
}
void EventListeners::cursor_button_handler(struct wl_listener* listener, void* data)
{
    (void)(listener);
    (void)(data);
}
void EventListeners::cursor_axis_handler(struct wl_listener* listener, void* data)
{
    (void)(listener);
    (void)(data);
}
void EventListeners::cursor_frame_handler(struct wl_listener* listener, void* data)
{
    (void)(listener);
    (void)(data);
}
void EventListeners::new_input_handler(struct wl_listener* listener, void* data)
{
    EventListeners* listeners = wl_container_of(listener, listeners, new_input);
    Server* server = listeners->server;
    auto* device = static_cast<struct wlr_input_device*>(data);

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        server->new_keyboard(device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        server->new_pointer(device);
        break;
    default:
        break;
    }

    // set pointer capability even if there is no mouse attached
    uint32_t capabilities = WL_SEAT_CAPABILITY_POINTER;
    if (!server->keyboards.empty()) {
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, capabilities);
}
void EventListeners::seat_request_cursor_handler(struct wl_listener* listener, void* data)
{
    EventListeners* listeners = wl_container_of(listener, listeners, request_cursor);
    Server* server = listeners->server;
    auto* event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
    struct wlr_seat_client* focused_client = server->seat->pointer_state.focused_client;

    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
    }
}
