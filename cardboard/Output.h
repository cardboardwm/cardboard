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

    struct wlr_box* get_box();
};

/// Executed for each frame render per output.
void output_frame_handler(struct wl_listener* listener, void* data);
/// Executed when the output is detached.
void output_destroy_handler(struct wl_listener* listener, void* data);
/// Executed when the output changes its mode (resolution, color depth and/or refresh rate).
void output_mode_handler(struct wl_listener* listener, void* data);
/// Executed when the output is transformed.
void output_transform_handler(struct wl_listener* listener, void* data);
/// Executed when the output is scaled.
void output_scale_handler(struct wl_listener* listener, void* data);

/// Registers event listeners and does bookkeeping for a newly added output.
void register_output(Server* server, Output&& output);
/// Arrange the workspace associated with \a output.
void arrange_output(Server* server, Output* output);

#endif // __CARDBOARD_OUTPUT_H_
