#ifndef __CARDBOARD_CURSOR_H_
#define __CARDBOARD_CURSOR_H_

struct Server;
struct Seat;

// TODO: rename to Cursor after namespacing all structs
struct SeatCursor {
    struct wlr_cursor* wlr_cursor;
    struct wlr_xcursor_manager* wlr_cursor_manager;

    Seat* seat;
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

#endif // __CARDBOARD_CURSOR_H_
