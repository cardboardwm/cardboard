#ifndef __CARDBOARD_OUTPUT_H_
#define __CARDBOARD_OUTPUT_H_

extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_output_layout.h>
}

#include <array>

#include "Layers.h"
#include "Server.h"

/**
 * \file
 * \brief This file contains listeners and helper functions for simple output operations.
 *
 * Outputs are displays.
 */

struct Output {
    struct wlr_output* wlr_output;
    struct wlr_box usable_area;

    /// Time of last presentation. Use it to calculate the delta time.
    struct timespec last_present;

    /// Executed for each frame render per output.
    static void frame_handler(struct wl_listener* listener, void* data);
    /// Executed as soon as the first pixel is put on the screen;
    static void present_handler(struct wl_listener* listener, void* data);
    /// Executed when the output is detached.
    static void destroy_handler(struct wl_listener* listener, void* data);
    /// Executed when the output changes its mode (resolution, color depth and/or refresh rate).
    static void mode_handler(struct wl_listener* listener, void* data);
    /// Executed when the output is transformed.
    static void transform_handler(struct wl_listener* listener, void* data);
    /// Executed when the output is scaled.
    static void scale_handler(struct wl_listener* listener, void* data);
};

/// Registers event listeners and does bookkeeping for a newly added output.
void register_output(Server& server, Output&& output);

#endif // __CARDBOARD_OUTPUT_H_
