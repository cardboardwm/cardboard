#ifndef __CARDBOARD_EVENT_LISTENERS_H_
#define __CARDBOARD_EVENT_LISTENERS_H_

#include <wayland-server.h>

/**
 * \file
 * \brief This file contains the Wayland event listeners for the
 * Server struct.
 *
 * Functions here implement \c wl_notify_func_t.
 */

/**
 * \brief Executed when a new output (monitor) is attached.
 *
 * It adds the output to Server::output_layout. The output
 * will be processed further after Server::output_layout signals
 * that the output has been added and configured.
 *
 * \sa output_layout_add_handler Called after Server::output_layout adds and configures the output.
 */
void new_output_handler(struct wl_listener* listener, void* data);

/**
 * \brief Processes the output after it has been configured by Server::output_layout.
 *
 * After that, the compositor stores it and registers some event handlers.
 * The compositor then assigns a workspace to this output, creating one if none is available.
 */
void output_layout_add_handler(struct wl_listener* listener, void* data);

/**
 * \brief Called when a new \c xdg_surface is created by a client.
 *
 * An \c xdg-surface is a type of surface exposed by the \c xdg-shell.
 * This handler creates a View based on it.
 */
void new_xdg_surface_handler(struct wl_listener* listener, void* data);

/**
 * \brief Called when a new \c layer_surface is created by a client.
 *
 * A \c layer_surface is a type of surface exposed by the \c layer-shell.
 * This handler does NOT create Views based on it.
 */
void new_layer_surface_handler(struct wl_listener* listener, void* data);

/**
 * \brief Called when an input device (keyboard, mouse, touchscreen, tablet) is attached.
 *
 * The device is registered within the compositor accordingly.
 */
void new_input_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_EVENT_LISTENERS_H_
