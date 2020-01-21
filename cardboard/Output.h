#ifndef __CARDBOARD_OUTPUT_H_
#define __CARDBOARD_OUTPUT_H_

#include <wayland-server.h>
#include <wlr_cpp/backend.h>

struct Server;

struct Output {
    Server* server;
    struct wlr_output* wlr_output;
    struct wl_listener frame;

    Output(Server* server, struct wlr_output* wlr_output);
    Output(const Output&) = delete;

    static void output_frame_handler(struct wl_listener* listener, void* data);
    static void render_surface(struct wlr_surface* surface, int sx, int sy, void* data);
};

#endif // __CARDBOARD_OUTPUT_H_
