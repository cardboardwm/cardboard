#include "Output.h"
#include "Listener.h"
#include "Server.h"

#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/render/wlr_renderer.h>
#include <wlr_cpp/types/wlr_matrix.h>
#include <wlr_cpp/types/wlr_output_layout.h>

#include <array>
#include <ctime>
#include <wlr_cpp/types/wlr_output.h>

struct RenderData {
    struct wlr_output* output;
    struct wlr_renderer* renderer;
    int ox, oy;
    const struct timespec* when;
    Server* server;
};

void register_output(Server* server, Output&& output_)
{
    server->outputs.emplace_back(output_);
    auto& output = server->outputs.back();
    output.wlr_output->data = &output;
    server->listeners.add_listener(&output.wlr_output->events.frame, Listener { output_frame_handler, server, &output });
    server->listeners.add_listener(&output.wlr_output->events.destroy, Listener { output_destroy_handler, server, &output });
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

    // translate surface coordinates to output coordinates
    double ox = rdata->ox + sx, oy = rdata->oy + sy;
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
    View* focused_view = server->get_focused_view();

    for (const auto& tile : ws.tiles) {
        if (!tile.view->mapped || tile.view == focused_view) {
            continue;
        }

        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .ox = tile.view->x,
            .oy = tile.view->y,
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
            .ox = focused_view->x,
            .oy = focused_view->y,
            .when = now,
            .server = server
        };

        focused_view->for_each_surface(render_surface, &rdata);
    }
}

static void render_floating(Server* server, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    // render stacked windows
    for (auto it = server->views.rbegin(); it != server->views.rend(); it++) {
        auto view = *it;
        if (!view->mapped || view->workspace_id >= 0) {
            // don't render unmapped views or views assigned to workspaces
            continue;
        }

        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .ox = view->x,
            .oy = view->y,
            .when = now,
            .server = server
        };

        view->for_each_surface(render_surface, &rdata);
    }
}

static void render_layer(Server* server, LayerArray::value_type& surfaces, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    for (const auto& surface : surfaces) {
        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .ox = surface.geometry.x,
            .oy = surface.geometry.y,
            .when = now,
            .server = server
        };

        wlr_surface_for_each_surface(surface.surface->surface, render_surface, &rdata);
    }
}

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

    auto& ws = *std::find_if(server->workspaces.begin(), server->workspaces.end(), [output](const auto& other) { return *other.output == output; });

    render_layer(server, output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], wlr_output, renderer, &now);
    render_layer(server, output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], wlr_output, renderer, &now);

    render_workspace(server, ws, wlr_output, renderer, &now);
    render_floating(server, wlr_output, renderer, &now);

    render_layer(server, output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], wlr_output, renderer, &now);
    render_layer(server, output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], wlr_output, renderer, &now);

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
        if (ws.output && *ws.output == output) {
            ws.deactivate();
            break;
        }
    }

    server->listeners.clear_listeners(output);
    server->outputs.remove_if([output](auto& other) { return &other == output; });
}
