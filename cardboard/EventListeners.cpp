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
        auto focused_workspace = server->seat.get_focused_workspace(server);
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

void new_input_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* device = static_cast<struct wlr_input_device*>(data);

    server->seat.add_input_device(server, device);
}
