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
 * \brief Called when a new \c xdg-surface is created by a client.
 *
 * An \c xdg-surface is a type of surface exposed by the \c xdg-shell.
 * This handler creates a View based on it.
 */
void new_xdg_surface_handler(struct wl_listener* listener, void* data);

/**
 * \brief Called when the cursor (mouse) is moved.
 *
 * In some cases, the mouse might "jump" to a position instead of moving by a certain delta.
 * This happens with the X11 backend for example. This handler only handles moves by a delta.
 *
 * \sa cursor_motion_absolute_handler for when the cursor "jumps".
 */
void cursor_motion_handler(struct wl_listener* listener, void* data);

/**
 * \brief Called when the cursor "jumps" or "warps" to an absolute position on the screen.
 *
 * \sa cursor_motion_handler for cursor moves by a delta.
 */
void cursor_motion_absolute_handler(struct wl_listener* listener, void* data);

/**
 * \brief Handles mouse button clicks.
 *
 * Besides transmitting clicks to the clients (by the means of Server::seat),
 * it also focuses the window under the cursor, with its side effects, such as auto-scrolling
 * the viewport of the Workspace the View under the cursor is in.
 */
void cursor_button_handler(struct wl_listener* listener, void* data);

/**
 * \brief Handles scrolling.
 */
void cursor_axis_handler(struct wl_listener* listener, void* data);

/**
 * \brief Handles pointer frame events.
 *
 * A frame event is sent after regular pointer events to group multiple events together.
 * For instance, two axis events may happend at the same time, in which case a frame event
 * won't be sent in between.
 */
void cursor_frame_handler(struct wl_listener* listener, void* data);

/**
 * \brief Called when an input device (keyboard, mouse, touchscreen, tablet) is attached.
 *
 * The device is registered within the compositor accordingly.
 */
void new_input_handler(struct wl_listener* listener, void* data);

/**
 * \brief Called when a client wants to set its own mouse cursor image.
 */
void seat_request_cursor_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_EVENT_LISTENERS_H_
