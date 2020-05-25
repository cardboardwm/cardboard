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

private:
    /// Called when the surface of the mouse pointer is destroyed by the client.
    static void image_surface_destroy_handler(struct wl_listener* listener, void* data);
};

void init_cursor(Server& server, SeatCursor& cursor);

/**
  * \brief Sets the cursor image to an xcursor named \a image.
  *
  * \param image - xcursor image name
  */
void cursor_set_image(Server& server, Seat& seat, SeatCursor& cursor, const char* image);
/**
  * \brief Sets the cursor image to a given \a surface with a given hotspot.
  *
  * The hotspot is the point of the cursor that interacts with the elements on the screen.
  * For a normal pointer, the hotspot is usually the tip of the cursor image.
  * For a cross pointer, the hotspot is usually in the center.
  */
void cursor_set_image_surface(Server& server, Seat& seat, SeatCursor& cursor, struct wlr_surface* surface, int32_t hotspot_x, int32_t hotspot_y);
/**
  * \brief Sets the cursor image according to the surface underneath,
  * and sends the appropriate cursor events to the surface underneath, if any.
  */
void cursor_rebase(Server& server, Seat& seat, SeatCursor& cursor, uint32_t time = 0);

/// Warps the cursor to the given output layout coordinates.
void cursor_warp(Server& server, Seat& seat, SeatCursor& cursor, int lx, int ly);

#endif // __CARDBOARD_CURSOR_H_
