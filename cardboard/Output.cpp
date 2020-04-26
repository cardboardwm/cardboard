#include "BuildConfig.h"

extern "C" {
#include <wayland-server.h>
#include <wlr/backend.h>
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#undef static
#include <wlr/types/wlr_output_layout.h>
}

#include <array>
#include <ctime>
#include <wlr/types/wlr_output.h>

#include "Listener.h"
#include "Output.h"
#include "Server.h"
#if HAVE_XWAYLAND
#include "Xwayland.h"
#endif

struct RenderData {
    struct wlr_output* output;
    struct wlr_renderer* renderer;
    int lx, ly;
    const struct timespec* when;
    Server* server;
};

void register_output(Server* server, Output&& output_)
{
    server->outputs.emplace_back(output_);
    auto& output = server->outputs.back();
    output.wlr_output->data = &output;

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &output.wlr_output->events.frame, output_frame_handler },
        { &output.wlr_output->events.mode, output_mode_handler },
        { &output.wlr_output->events.transform, output_transform_handler },
        { &output.wlr_output->events.scale, output_scale_handler },
        { &output.wlr_output->events.destroy, output_destroy_handler }
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, server, &output });
    }
}

void arrange_output(Server* server, Output* output)
{
    auto ws_it = std::find_if(server->workspaces.begin(), server->workspaces.end(), [output](const auto& other) { return other.output && &other.output.unwrap() == output; });
    if (ws_it == server->workspaces.end()) {
        return;
    }
    ws_it->arrange_tiles();
}

void render_surface(struct wlr_surface* surface, int sx, int sy, void* data)
{
    auto* rdata = static_cast<RenderData*>(data);
    Server* server = rdata->server;
    struct wlr_output* output = rdata->output;

    struct wlr_texture* texture = wlr_surface_get_texture(surface);
    if (texture == nullptr) {
        return;
    }

    // translate surface coordinates from layout-relative to output-relative coordinates
    double ox = rdata->lx + sx, oy = rdata->ly + sy;
    wlr_output_layout_output_coords(server->output_layout, output, &ox, &oy);

    // apply scale factor for HiDPI outputs
    struct wlr_box box = {
        .x = static_cast<int>(ox * output->scale),
        .y = static_cast<int>(oy * output->scale),
        .width = static_cast<int>(surface->current.width * output->scale),
        .height = static_cast<int>(surface->current.height * output->scale),
    };

    // project box on ortographic projection
    std::array<float, 9> matrix;
    enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
    wlr_matrix_project_box(matrix.data(), &box, transform, 0, output->transform_matrix);

    wlr_render_texture_with_matrix(rdata->renderer, texture, matrix.data(), 1);
    wlr_surface_send_frame_done(surface, rdata->when);
}

static void render_workspace(Server* server, Workspace& ws, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    View* focused_view = server->seat.get_focused_view();

    for (const auto& tile : ws.tiles) {
        if (!tile.view->mapped || tile.view == focused_view) {
            continue;
        }

        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = tile.view->x,
            .ly = tile.view->y,
            .when = now,
            .server = server
        };

        tile.view->for_each_surface(render_surface, &rdata);
    }

    // render the focused view last (only if it's tiled)
    if (focused_view && focused_view->workspace_id == ws.index) {
        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = focused_view->x,
            .ly = focused_view->y,
            .when = now,
            .server = server
        };

        focused_view->for_each_surface(render_surface, &rdata);
    }
}

static void render_floating(Server* server, View* ancestor, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    // render stacked windows
    for (auto it = server->views.rbegin(); it != server->views.rend(); it++) {
        auto view = *it;
        if (!view->mapped || view->workspace_id >= 0 || (ancestor && !view->is_transient_for(ancestor))) {
            // don't render unmapped views or views assigned to workspaces
            continue;
        }

        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = view->x,
            .ly = view->y,
            .when = now,
            .server = server
        };

        view->for_each_surface(render_surface, &rdata);
    }
}

static void render_layer(Server* server, LayerArray::value_type& surfaces, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    auto* output_box = wlr_output_layout_get_box(server->output_layout, wlr_output);
    for (const auto& surface : surfaces) {
        if (!surface.mapped || !surface.is_on_output(static_cast<Output*>(wlr_output->data))) {
            continue;
        }

        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = surface.geometry.x + output_box->x,
            .ly = surface.geometry.y + output_box->y,
            .when = now,
            .server = server
        };

        wlr_layer_surface_v1_for_each_surface(surface.surface, render_surface, &rdata);
    }
}

#if HAVE_XWAYLAND
static void render_xwayland_or_surface(Server* server, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    auto* output_box = wlr_output_layout_get_box(server->output_layout, wlr_output);
    for (const auto xwayland_or_surface : server->xwayland_or_surfaces) {
        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = xwayland_or_surface->lx - output_box->x,
            .ly = xwayland_or_surface->ly - output_box->y,
            .when = now,
            .server = server
        };

        wlr_surface_for_each_surface(xwayland_or_surface->xwayland_surface->surface, render_surface, &rdata);
    }
}
#endif

void output_frame_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Server* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);
    auto* wlr_output = output->wlr_output;
    struct wlr_renderer* renderer = server->renderer;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // make the OpenGL context current
    if (!wlr_output_attach_render(wlr_output, nullptr)) {
        return;
    }

    int width, height;
    // the effective resolution takes the rotation of outputs in account
    wlr_output_effective_resolution(wlr_output, &width, &height);
    wlr_renderer_begin(renderer, width, height);

    std::array<float, 4> color = { .3, .3, .3, 1. };
    wlr_renderer_clear(renderer, color.data());

    auto& ws = *std::find_if(server->workspaces.begin(), server->workspaces.end(), [output](const auto& other) { return other.output && &other.output.unwrap() == output; });
    if (ws.fullscreen_view) {
        render_workspace(server, ws, wlr_output, renderer, &now);
#if HAVE_XWAYLAND
        render_xwayland_or_surface(server, wlr_output, renderer, &now);
#endif
        render_floating(server, &ws.fullscreen_view.unwrap(), wlr_output, renderer, &now);
    } else {
        render_layer(server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], wlr_output, renderer, &now);
        render_layer(server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], wlr_output, renderer, &now);

        wlr_renderer_scissor(renderer, &output->usable_area);
        render_workspace(server, ws, wlr_output, renderer, &now);
        wlr_renderer_scissor(renderer, nullptr);

#if HAVE_XWAYLAND
        render_xwayland_or_surface(server, wlr_output, renderer, &now);
#endif

        render_floating(server, nullptr, wlr_output, renderer, &now);
        render_layer(server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], wlr_output, renderer, &now);
    }
    render_layer(server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], wlr_output, renderer, &now);

    // in case of software rendered cursor, render it
    wlr_output_render_software_cursors(wlr_output, nullptr);

    // swap buffers and show frame
    wlr_renderer_end(renderer);
    wlr_output_commit(wlr_output);
}

void output_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Server* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    for (auto& ws : server->workspaces) {
        if (ws.output && &ws.output.unwrap() == output) {
            ws.deactivate();
            break;
        }
    }

    server->listeners.clear_listeners(output);
    server->outputs.remove_if([output](auto& other) { return &other == output; });
}

void output_mode_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    arrange_layers(server, output);
    arrange_output(server, output);
}

void output_transform_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    arrange_layers(server, output);
    arrange_output(server, output);
}

void output_scale_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    arrange_layers(server, output);
    arrange_output(server, output);
}
