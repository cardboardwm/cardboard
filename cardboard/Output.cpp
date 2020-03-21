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
    const View* view;
    const struct timespec* when;
    Server* server;
};

void register_output(Server* server, wlr_output* output)
{
    server->outputs.emplace_back(output);
    server->listeners.add_listener(&output->events.frame, Listener { output_frame_handler, server, output });
    server->listeners.add_listener(&output->events.destroy, Listener { output_destroy_handler, server, output });
}

void render_surface(struct wlr_surface* surface, int sx, int sy, void* data)
{
    auto* rdata = static_cast<RenderData*>(data);
    const View* view = rdata->view;
    Server* server = rdata->server;
    struct wlr_output* output = rdata->output;

    struct wlr_texture* texture = wlr_surface_get_texture(surface);
    if (texture == nullptr) {
        return;
    }

    // translate surface coordinates to output coordinates
    double ox = 0, oy = 0;
    wlr_output_layout_output_coords(server->output_layout, output, &ox, &oy);
    ox += view->x + sx;
    oy += view->y + sy;

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

void output_frame_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Server* server = get_server(listener);
    wlr_output* output = get_listener_data<wlr_output*>(listener);
    struct wlr_renderer* renderer = server->renderer;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // make the OpenGL context current
    if (!wlr_output_attach_render(output, nullptr)) {
        return;
    }

    int width, height;
    // the effective resolution takes the rotation of outputs in account
    wlr_output_effective_resolution(output, &width, &height);
    wlr_renderer_begin(renderer, width, height);

    std::array<float, 4> color = { .3, .3, .3, 1. };
    wlr_renderer_clear(renderer, color.data());

    for (auto view = server->views.rbegin(); view != server->views.rend(); view++) {
        if (!view->mapped) {
            // don't render unmapped views
            continue;
        }

        RenderData rdata = {
            .output = output,
            .renderer = renderer,
            .view = &(*view),
            .when = &now,
            .server = server
        };

        wlr_xdg_surface_for_each_surface(view->xdg_surface, render_surface, &rdata);
    }

    // in case of software rendered cursor, render it
    wlr_output_render_software_cursors(output, nullptr);

    // swap buffers and show frame
    wlr_renderer_end(renderer);
    wlr_output_commit(output);
}

void output_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Server* server = get_server(listener);
    wlr_output* output = get_listener_data<wlr_output*>(listener);

    server->listeners.clear_listeners(output);
}
