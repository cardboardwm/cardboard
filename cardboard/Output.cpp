// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "BuildConfig.h"

extern "C" {
#include <wayland-server.h>
#include <wlr/backend.h>
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#undef static
#include <wlr/types/wlr_output_layout.h>
#include <wlr/util/log.h>
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
    server.output_manager->outputs.emplace_back(output_);
    auto& output = server.output_manager->outputs.back();
    output.wlr_output->data = &output;
    output.wlr_output_damage = wlr_output_damage_create(output.wlr_output);

    register_handlers(server,
                      &output,
                      {
                          { &output.wlr_output_damage->events.frame, Output::frame_handler },
                          { &output.wlr_output->events.present, Output::present_handler },
                          { &output.wlr_output->events.commit, Output::commit_handler },
                          { &output.wlr_output->events.mode, Output::mode_handler },
                          { &output.wlr_output->events.destroy, Output::destroy_handler },
                      });
}

/// Arrange the workspace associated with \a output.
static void arrange_output(Server& server, Output& output)
{
    auto ws_it = std::find_if(server.output_manager->workspaces.begin(), server.output_manager->workspaces.end(), [&output](const auto& other) { return other.output && other.output.raw_pointer() == &output; });
    if (ws_it == server.output_manager->workspaces.end()) {
        return;
    }
    ws_it->arrange_workspace(*(server.output_manager));
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
    wlr_output_layout_output_coords(server->output_manager->output_layout, output, &ox, &oy);

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
    for (const auto& column : ws.columns) {
        for (const auto& tile : column.tiles) {
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
        if ((!view->mapped) || (ancestor && !view->is_transient_for(ancestor.unwrap()) && view != ancestor.raw_pointer())) {
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
    const struct wlr_box* output_box = server.output_manager->get_output_box(output);
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
    for (const auto& xwayland_or_surface : server.surface_manager.xwayland_or_surfaces) {
        if (!xwayland_or_surface->mapped || !xwayland_or_surface->xwayland_surface->surface) {
            continue;
        }
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

    server->seat.update_swipe(*server);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // make the OpenGL context current
    bool needs_frame;
    pixman_region32_t damage;
    pixman_region32_init(&damage);
    if (!wlr_output_damage_attach_render(output->wlr_output_damage, &needs_frame, &damage)) {
        wlr_log(WLR_ERROR, "cannot make damage output current");
        return;
    }

    if (!needs_frame) {
        wlr_output_rollback(output->wlr_output);
        goto damage_finish;
    }

    int width, height;
    // the effective resolution takes the rotation of outputs in account
    wlr_output_effective_resolution(wlr_output, &width, &height);
    wlr_renderer_begin(renderer, width, height);

    {
        std::array<float, 4> color = { .3, .3, .3, 1. };
        wlr_renderer_clear(renderer, color.data());
    }

    {
        int workspaces_number = 0;
        int fullscreen_workspaces_number = 0;

        for (auto& ws : server->output_manager->workspaces) {
            if (ws.output.raw_pointer() == output) {
                workspaces_number++;
                fullscreen_workspaces_number += static_cast<bool>(ws.fullscreen_view);
            }
        }

        if (fullscreen_workspaces_number != workspaces_number - fullscreen_workspaces_number) {
            render_layer(*server, server->surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], *output, renderer, &now);
            render_layer(*server, server->surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], *output, renderer, &now);
        }
    }

    for (auto& ws : server->output_manager->workspaces) {

        if (ws.output.raw_pointer() != output) {
            continue;
        }

        if (ws.fullscreen_view) {
            render_workspace(*server, ws, wlr_output, renderer, &now);
#if HAVE_XWAYLAND
            render_xwayland_or_surface(*server, wlr_output, renderer, &now);
#endif
            render_floating(*server, ws, ws.fullscreen_view, wlr_output, renderer, &now);
        } else {

            if (auto focused_view_ptr = server->seat.get_focused_view(); focused_view_ptr) {
                auto focused_view = focused_view_ptr.raw_pointer();

                if (auto column_it = ws.find_column(focused_view); column_it != ws.columns.end()) {
                    wlr_box column_dimensions = {
                        .x = focused_view->x + focused_view->geometry.x - server->config.gap / 2,
                        .y = focused_view->y + focused_view->geometry.y - server->config.gap / (column_it->tiles.size() == 1 ? 1 : 2),
                        .width = focused_view->target_width + server->config.gap,
                        .height = focused_view->target_height + (column_it->tiles.size() == 1 ? 2 : 1) * server->config.gap
                    };

                    std::array<float, 9> matrix;
                    wl_output_transform transform = wlr_output_transform_invert(
                        focused_view->get_surface()->current.transform);
                    wlr_matrix_project_box(matrix.data(), &column_dimensions, transform, 0, wlr_output->transform_matrix);

                    auto focus_color = server->config.focus_color;
                    // premultiply components
                    focus_color.r *= focus_color.a;
                    focus_color.g *= focus_color.a;
                    focus_color.b *= focus_color.a;
                    wlr_render_quad_with_matrix(
                        renderer,
                        reinterpret_cast<float*>(&focus_color),
                        matrix.data());
                }
            }

            render_workspace(*server, ws, wlr_output, renderer, &now);
            wlr_renderer_scissor(renderer, nullptr);

#if HAVE_XWAYLAND
            render_xwayland_or_surface(*server, wlr_output, renderer, &now);
#endif

            render_floating(*server, ws, NullRef<View>, wlr_output, renderer, &now);
            render_layer(*server, server->surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], *output, renderer, &now);
        }
    }

    render_layer(*server, server->surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], *output, renderer, &now);

    // in case of software rendered cursor, render it
    wlr_output_render_software_cursors(wlr_output, nullptr);

    // swap buffers and show frame
    wlr_renderer_end(renderer);
    wlr_output_commit(wlr_output);

damage_finish:
    pixman_region32_fini(&damage);
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

    for (auto& ws : server->output_manager->workspaces) {
        if (ws.output && &ws.output.unwrap() == output) {
            ws.deactivate();
            break;
        }
    }

    server->listeners.clear_listeners(output);
    server->output_manager->remove_output_from_list(*output);
}

void Output::commit_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);
    auto* event = static_cast<struct wlr_output_event_commit*>(data);

    if (event->committed & (WLR_OUTPUT_STATE_SCALE | WLR_OUTPUT_STATE_TRANSFORM)) {
        arrange_layers(*server, *output);
        arrange_output(*server, *output);
    }
}

void Output::mode_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* output = get_listener_data<Output*>(listener);

    arrange_layers(*server, *output);
    arrange_output(*server, *output);
}
