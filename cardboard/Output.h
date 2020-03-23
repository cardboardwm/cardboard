#ifndef __CARDBOARD_OUTPUT_H_
#define __CARDBOARD_OUTPUT_H_

#include "Server.h"
#include <wayland-server.h>
#include <wlr_cpp/types/wlr_output_layout.h>

/**
 * \file
 * \brief This file contains listeners and helper functions for simple output operations.
 *
 * Outputs are displays.
 */

/// Executed for each frame render per output.
void output_frame_handler(struct wl_listener* listener, void* data);
/// Executed when the output is detached.
void output_destroy_handler(struct wl_listener* listener, void* data);

/// Registers event listeners and does bookkeeping for a newly added output.
void register_output(Server* server, wlr_output_layout_output*);

#endif // __CARDBOARD_OUTPUT_H_
