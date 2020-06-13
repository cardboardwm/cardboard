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

#include "Helpers.h"
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

void register_output(Server& server, Output&& output_)
{
    server.output_manager.outputs.emplace_back(output_);
    auto& output = server.output_manager.outputs.back();
    output.wlr_output->data = &output;

    register_handlers(server,
                      &output,
                      {
                          { &output.wlr_output->events.frame, Output::frame_handler },
                          { &output.wlr_output->events.present, Output::present_handler },
                          { &output.wlr_output->events.mode, Output::mode_handler },
                          { &output.wlr_output->events.transform, Output::transform_handler },
                          { &output.wlr_output->events.scale, Output::scale_handler },
                          { &output.wlr_output->events.destroy, Output::destroy_handler },
                      });
}

/// Arrange the workspace associated with \a output.
static void arrange_output(Server& server, Output& output)
{
    auto ws_it = std::find_if(server.workspaces.begin(), server.workspaces.end(), [&output](const auto& other) { return other.output && other.output.raw_pointer() == &output; });
    if (ws_it == server.workspaces.end()) {
        return;
    }
    ws_it->arrange_workspace(server.output_manager);
}

static void render_surface(struct wlr_surface* surface, int sx, int sy, void* data)
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
    wlr_output_layout_output_coords(server->output_manager.output_layout, output, &ox, &oy);

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

static void render_workspace(Server& server, Workspace& ws, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    auto focused_view = server.seat.get_focused_view();

    bool focused_tiled = false;
    for (const auto& tile : ws.tiles) {
        if (!tile.view->mapped) {
            continue;
        }

        if (tile.view == focused_view.raw_pointer()) {
            focused_tiled = true;
            continue;
        }

        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = tile.view->x,
            .ly = tile.view->y,
            .when = now,
            .server = &server
        };

        tile.view->for_each_surface(render_surface, &rdata);
    }

    if (focused_tiled) {
        auto& focused_view_r = focused_view.unwrap();
        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = focused_view_r.x,
            .ly = focused_view_r.y,
            .when = now,
            .server = &server
        };

        focused_view_r.for_each_surface(render_surface, &rdata);
    }
}

static void render_floating(Server& server, Workspace& ws, OptionalRef<View> ancestor, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    auto focused_view = server.seat.get_focused_view();

    bool focused_floating = false;
    for (const auto& view : ws.floating_views) {
        if ((!view->mapped) || (ancestor && !view->is_transient_for(ancestor.unwrap()))) {
            continue;
        }

        if (view == focused_view.raw_pointer()) {
            focused_floating = true;
            continue;
        }

        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = view->x,
            .ly = view->y,
            .when = now,
            .server = &server
        };

        view->for_each_surface(render_surface, &rdata);
    }

    if (focused_floating) {
        auto& focused_view_r = focused_view.unwrap();
        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = focused_view_r.x,
            .ly = focused_view_r.y,
            .when = now,
            .server = &server
        };

        focused_view_r.for_each_surface(render_surface, &rdata);
    }
}

static void render_layer(Server& server, LayerArray::value_type& surfaces, Output& output, struct wlr_renderer* renderer, struct timespec* now)
{
    const struct wlr_box* output_box = server.output_manager.get_output_box(output);
    for (const auto& surface : surfaces) {
        if (!surface.surface->mapped || !surface.is_on_output(output)) {
            continue;
        }

        RenderData rdata = {
            .output = output.wlr_output,
            .renderer = renderer,
            .lx = surface.geometry.x + output_box->x,
            .ly = surface.geometry.y + output_box->y,
            .when = now,
            .server = &server
        };

        wlr_layer_surface_v1_for_each_surface(surface.surface, render_surface, &rdata);
    }
}

#if HAVE_XWAYLAND
static void render_xwayland_or_surface(Server& server, struct wlr_output* wlr_output, struct wlr_renderer* renderer, struct timespec* now)
{
    for (const auto xwayland_or_surface : server.xwayland_or_surfaces) {
        RenderData rdata = {
            .output = wlr_output,
            .renderer = renderer,
            .lx = xwayland_or_surface->lx,
            .ly = xwayland_or_surface->ly,
            .when = now,
            .server = &server
        };

        wlr_surface_for_each_surface(xwayland_or_surface->xwayland_surface->surface, render_surface, &rdata);
    }
}
#endif

/// Not used yet but it's going to be useful one day.
[[maybe_unused]] static double delta_time(struct timespec& x, struct timespec& y)
{
    struct timespec delta;

    // the numbers with many zeros are one billion.
    // we are dealing with nanoseconds.
    if (x.tv_nsec < y.tv_nsec) {
        int secs = (y.tv_nsec - x.tv_nsec) / 1000000000 + 1;
        y.tv_nsec -= 1000000000 * secs;
        y.tv_sec += secs;
    }

    if (x.tv_nsec - y.tv_nsec > 1000000000) {
        int secs = (x.tv_nsec - y.tv_nsec) / 1000000000;
        y.tv_nsec += 1000000000 * secs;
        y.tv_sec -= secs;
    }

    delta.tv_sec = x.tv_sec - y.tv_sec;
    delta.tv_nsec = x.tv_nsec - y.tv_nsec;

    return static_cast<double>(delta.tv_sec) + static_cast<double>(delta.tv_nsec) / 1000000000.0;
}

void Output::frame_handler(struct wl_listener* listener, void*)
{
    Server* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);
    auto* wlr_output = output->wlr_output;
    struct wlr_renderer* renderer = server->renderer;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    server->seat.update_swipe(*server);

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

    auto& ws = *std::find_if(server->workspaces.begin(), server->workspaces.end(), [output](const auto& other) { return other.output && other.output.raw_pointer() == output; });
    if (ws.fullscreen_view) {
        render_workspace(*server, ws, wlr_output, renderer, &now);
#if HAVE_XWAYLAND
        render_xwayland_or_surface(*server, wlr_output, renderer, &now);
#endif
        render_floating(*server, ws, ws.fullscreen_view, wlr_output, renderer, &now);
    } else {
        render_layer(*server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], *output, renderer, &now);
        render_layer(*server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], *output, renderer, &now);

        wlr_renderer_scissor(renderer, &output->usable_area);
        render_workspace(*server, ws, wlr_output, renderer, &now);
        wlr_renderer_scissor(renderer, nullptr);

#if HAVE_XWAYLAND
        render_xwayland_or_surface(*server, wlr_output, renderer, &now);
#endif

        render_floating(*server, ws, NullRef<View>, wlr_output, renderer, &now);
        render_layer(*server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], *output, renderer, &now);
    }
    render_layer(*server, server->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], *output, renderer, &now);

    // in case of software rendered cursor, render it
    wlr_output_render_software_cursors(wlr_output, nullptr);

    // swap buffers and show frame
    wlr_renderer_end(renderer);
    wlr_output_commit(wlr_output);
}

void Output::present_handler(struct wl_listener* listener, void* data)
{
    auto* output = get_listener_data<Output*>(listener);
    auto* event = static_cast<struct wlr_output_event_present*>(data);

    output->last_present = *event->when;
}

void Output::destroy_handler(struct wl_listener* listener, void*)
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
    server->output_manager.remove_output_from_list(*output);
}

void Output::mode_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    arrange_layers(*server, *output);
    arrange_output(*server, *output);
}

void Output::transform_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    arrange_layers(*server, *output);
    arrange_output(*server, *output);
}

void Output::scale_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    arrange_layers(*server, *output);
    arrange_output(*server, *output);
}
