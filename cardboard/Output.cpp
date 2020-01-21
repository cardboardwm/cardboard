#include "Output.h"
#include "Server.h"

#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/render/wlr_renderer.h>
#include <wlr_cpp/types/wlr_matrix.h>
#include <wlr_cpp/types/wlr_output.h>
#include <wlr_cpp/types/wlr_output_layout.h>

#include <array>
#include <ctime>

struct RenderData {
    struct wlr_output* output;
    struct wlr_renderer* renderer;
    const View* view;
    const struct timespec* when;
};

Output::Output(Server* server, struct wlr_output* wlr_output)
    : server(server)
    , wlr_output(wlr_output)
{
    frame.notify = Output::output_frame_handler;
    wl_signal_add(&wlr_output->events.frame, &frame);
}

void Output::render_surface(struct wlr_surface* surface, int sx, int sy, void* data)
{
    auto* rdata = static_cast<RenderData*>(data);
    const View* view = rdata->view;
    struct wlr_output* output = rdata->output;

    struct wlr_texture* texture = wlr_surface_get_texture(surface);
    if (texture == nullptr) {
        return;
    }

    // translate surface coordinates to output coordinates
    double ox = 0, oy = 0;
    wlr_output_layout_output_coords(view->server->output_layout, output, &ox, &oy);
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

void Output::output_frame_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Output* output = wl_container_of(listener, output, frame);
    struct wlr_renderer* renderer = output->server->renderer;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // make the OpenGL context current
    if (!wlr_output_attach_render(output->wlr_output, nullptr)) {
        return;
    }

    int width, height;
    // the effective resolution takes the rotation of outputs in account
    wlr_output_effective_resolution(output->wlr_output, &width, &height);
    wlr_renderer_begin(renderer, width, height);

    std::array<float, 4> color = { .3, .3, .3, 1. };
    wlr_renderer_clear(renderer, color.data());

    for (const auto& view : output->server->views) {
        if (!view.mapped) {
            // don't render unmapped views
            continue;
        }

        RenderData rdata = {
            .output = output->wlr_output,
            .view = &view,
            .renderer = renderer,
            .when = &now
        };

        wlr_xdg_surface_for_each_surface(view.xdg_surface, Output::render_surface, &rdata);
    }

    // in case of software rendered cursor, render it
    wlr_output_render_software_cursors(output->wlr_output, nullptr);

    // swap buffers and show frame
    wlr_renderer_end(renderer);
    wlr_output_commit(output->wlr_output);
}
