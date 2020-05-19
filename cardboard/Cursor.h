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

    /**
     * \brief Sets the cursor image to an xcursor named \a image.
     *
     * \param image - xcursor image name
     */
    void set_image(Server* server, const char* image);
    /**
     * \brief Sets the cursor image to a given \a surface with a given hotspot.
     *
     * The hotspot is the point of the cursor that interacts with the elements on the screen.
     * For a normal pointer, the hotspot is usually the tip of the cursor image.
     * For a cross pointer, the hotspot is usually in the center.
     */
    void set_image_surface(Server* server, struct wlr_surface* surface, int32_t hotspot_x, int32_t hotspot_y);
    /**
     * \brief Sets the cursor image according to the surface underneath,
     * and sends the appropriate cursor events to the surface underneath, if any.
     */
    void rebase(Server* server, uint32_t time = 0);

    /// Warps the cursor to the given output layout coordinates.
    void warp(Server* server, int lx, int ly) const;

private:
    /**
     * \brief Registers a destroy handler (SeatCursor::image_surface_destroy_handler) on \a surface
     * to correctly unset the surface as the cursor after it gets destroyed.
     */
    void register_image_surface(Server* server, struct wlr_surface* surface);

public:
    /**
    * \brief Called when the cursor (mouse) is moved.
    *
    * In some cases, the mouse might "jump" to a position instead of moving by a certain delta.
    * This happens with the X11 backend for example. This handler only handles moves by a delta.
    *
    * \sa cursor_motion_absolute_handler for when the cursor "jumps".
    */
    static void motion_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when the cursor "jumps" or "warps" to an absolute position on the screen.
    *
    * \sa cursor_motion_handler for cursor moves by a delta.
    */
    static void motion_absolute_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Handles mouse button clicks.
    *
    * Besides transmitting clicks to the clients (by the means of Server::seat),
    * it also focuses the window under the cursor, with its side effects, such as auto-scrolling
    * the viewport of the Workspace the View under the cursor is in.
    */
    static void button_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Handles scrolling.
    */
    static void axis_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Handles pointer frame events.
    *
    * A frame event is sent after regular pointer events to group multiple events together.
    * For instance, two axis events may happend at the same time, in which case a frame event
    * won't be sent in between.
    */
    static void frame_handler(struct wl_listener* listener, void* data);

    /**
     * \brief Called when the user starts a swipe on the touchpad (more than one finger).
     */
    static void swipe_update_handler(struct wl_listener* listener, void* data);
    /**
     * \brief Called when the user moved their fingers during a swipe on the touchpad.
     */
    static void swipe_begin_handler(struct wl_listener* listener, void* data);
    static void swipe_end_handler(struct wl_listener* listener, void* data);

private:
    /// Called when the surface of the mouse pointer is destroyed by the client.
    static void image_surface_destroy_handler(struct wl_listener* listener, void* data);
};

void init_cursor(Server* server, Seat* seat, SeatCursor* cursor);

#endif // __CARDBOARD_CURSOR_H_
