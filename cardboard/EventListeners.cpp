#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_cursor.h>
#include <wlr_cpp/types/wlr_output.h>
#include <wlr_cpp/types/wlr_output_layout.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>
#include <wlr_cpp/util/log.h>

// keep empty line above to avoid header sorting for this little guy
#include <layer_shell_v1.h>

#include "Layers.h"
#include "Listener.h"
#include "Server.h"

#include "EventListeners.h"

void new_output_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* output = static_cast<struct wlr_output*>(data);

    // pick the monitor's preferred mode
    if (!wl_list_empty(&output->modes)) {
        struct wlr_output_mode* mode = wlr_output_preferred_mode(output);
        wlr_output_set_mode(output, mode);
        wlr_output_enable(output, true);
        if (!wlr_output_commit(output)) {
            return;
        }
    }

    // add output to the layout. add_auto arranges outputs left-to-right
    // in the order they appear.
    wlr_output_layout_add_auto(server->output_layout, output);
}

void output_layout_add_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* l_output = static_cast<struct wlr_output_layout_output*>(data);

    auto output_ = Output {};
    output_.wlr_output = l_output->output;
    wlr_output_effective_resolution(output_.wlr_output, &output_.usable_area.width, &output_.usable_area.height);
    register_output(server, std::move(output_));

    auto& output = server->outputs.back();

    Workspace* ws_to_assign;
    for (auto& ws : server->workspaces) {
        if (!ws.output) {
            ws_to_assign = &ws;
            break;
        }
    }

    if (!ws_to_assign) {
        ws_to_assign = &server->create_workspace();
    }

    ws_to_assign->output = &output;

    // expose the output to the clients
    wlr_output_create_global(output.wlr_output);
}

void new_xdg_surface_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* xdg_surface = static_cast<struct wlr_xdg_surface*>(data);

    // ignore popups TODO: don't ignore them
    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    create_view(server, new XDGView(xdg_surface));
}

void new_layer_surface_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* layer_surface = static_cast<struct wlr_layer_surface_v1*>(data);
    wlr_log(WLR_DEBUG,
            "new layer surface: namespace %s layer %d anchor %d "
            "size %dx%d margin %d,%d,%d,%d",
            layer_surface->namespace_,
            layer_surface->client_pending.layer,
            layer_surface->client_pending.anchor,
            layer_surface->client_pending.desired_width,
            layer_surface->client_pending.desired_height,
            layer_surface->client_pending.margin.top,
            layer_surface->client_pending.margin.right,
            layer_surface->client_pending.margin.bottom,
            layer_surface->client_pending.margin.left);

    if (!layer_surface->output) {
        // Assigns output of the focused workspace
        Output* output = nullptr;
        auto focused_workspace = server->get_focused_workspace();
        if (focused_workspace && focused_workspace->get().output) {
            output = *focused_workspace->get().output;
        }
        if (!output) {
            if (server->outputs.empty()) {
                wlr_layer_surface_v1_close(layer_surface);
                return;
            }
            output = &server->outputs.front();
        }
        layer_surface->output = output->wlr_output;
    }

    LayerSurface ls;
    ls.surface = layer_surface;
    create_layer(server, std::move(ls));
}

void cursor_motion_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* event = static_cast<struct wlr_event_pointer_motion*>(data);
    wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
    server->process_cursor_motion(event->time_msec);
}

void cursor_motion_absolute_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
    wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
    server->process_cursor_motion(event->time_msec);
}

void cursor_button_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* event = static_cast<struct wlr_event_pointer_button*>(data);

    wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
    double sx, sy;
    struct wlr_surface* surface;
    View* view = server->get_surface_under_cursor(server->cursor->x, server->cursor->y, surface, sx, sy);
    if (event->state == WLR_BUTTON_RELEASED) {
        // end grabbing
        server->grab_state = std::nullopt;
    } else if (view != server->get_focused_view()) {
        server->focus_view(view);
    }
}

void cursor_axis_handler(struct wl_listener* listener, void* data)
{
    // this happens when you scroll
    Server* server = get_server(listener);

    auto* event = static_cast<struct wlr_event_pointer_axis*>(data);
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}

void cursor_frame_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Server* server = get_server(listener);

    wlr_seat_pointer_notify_frame(server->seat);
}

void new_input_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* device = static_cast<struct wlr_input_device*>(data);

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        server->new_keyboard(device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        server->new_pointer(device);
        break;
    default:
        // TODO: touchscreens and drawing tablets
        break;
    }

    // set pointer capability even if there is no mouse attached
    uint32_t capabilities = WL_SEAT_CAPABILITY_POINTER;
    if (!server->keyboards.empty()) {
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, capabilities);
}

void seat_request_cursor_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
    struct wlr_seat_client* focused_client = server->seat->pointer_state.focused_client;

    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
    }
}
