#ifndef __CARDBOARD_CURSOR_H_
#define __CARDBOARD_CURSOR_H_

extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
}

#include <cstdint>

#include "OptionalRef.h"

struct Server;
struct Seat;

// TODO: rename to Cursor after namespacing all structs
struct SeatCursor {
    struct wlr_cursor* wlr_cursor;
    struct wlr_xcursor_manager* wlr_xcursor_manager;
    OptionalRef<struct wl_listener> image_surface_destroy_listener;

    Seat* seat;

    void set_image(Server* server, const char* image);
    void set_image_surface(Server* server, struct wlr_surface* surface, int32_t hotspot_x, int32_t hotspot_y);
    /// Sets the cursor image according to the surface underneath.
    void rebase(Server* server, uint32_t time = 0);

    void move(Server* server, int x, int y) const;

private:
    void register_image_surface(Server* server, struct wlr_surface* surface);
};

void init_cursor(Server* server, Seat* seat, SeatCursor* cursor);

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

/// Called when the surface of the mouse pointer is destroyed by the client.
void cursor_image_surface_destroy_handler(struct wl_listener* listener, void* data);

void cursor_swipe_update_handler(struct wl_listener* listener, void* data);
void cursor_swipe_begin_handler(struct wl_listener* listener, void* data);
void cursor_swipe_end_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_CURSOR_H_
