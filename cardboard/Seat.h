#ifndef __CARDBOARD_SEAT_H_
#define __CARDBOARD_SEAT_H_

#include <wayland-server-core.h>
#include <wlr_cpp/types/wlr_input_device.h>
#include <wlr_cpp/types/wlr_seat.h>

#include <functional>
#include <list>
#include <optional>
#include <vector>

#include "Cursor.h"
#include "Keyboard.h"
#include "View.h"

struct Server;

constexpr const char* DEFAULT_SEAT = "seat0";

struct Seat {
    struct GrabState {
        enum class Mode {
            MOVE,
            RESIZE
        } mode;
        View* view;
        double x, y;
        int width, height;
        uint32_t resize_edges;
    };

    SeatCursor cursor;
    std::optional<GrabState> grab_state;

    struct wlr_seat* wlr_seat;

    std::list<Keyboard> keyboards;
    std::list<View*> focus_stack; ///< Views ordered by the time they were focused, from most recent.

    /// Sets up a newly attached input device.
    void add_input_device(Server* server, struct wlr_input_device* device);
    /// Sets up a new keyboard device, with events and keymap.
    void add_keyboard(Server* server, struct wlr_input_device* device);
    /// Attaches a new pointer device (Server* server, e.g. mouse) to the cursor.
    void add_pointer(Server* server, struct wlr_input_device* device);

    /// Returns the currently focused View. It is defined as the View currently holding keyboard focus.
    View* get_focused_view();
    /// Hides the \a view from the screen without unmapping. Happens when a Workspace is deactivated.
    void hide_view(Server* server, View* view);
    /// Sets the focus state on \a view. Auto-scrolls the Workspace if it's tiled.
    void focus_view(Server* server, View* view);
    /**
     * \brief Focus the <em>offset</em>-nth tiled window to the right (or to the left if negative) of the currently
     * focused view.
     */
    void focus_by_offset(Server* server, int offset);
    /// Removes the \a view from the focus stack.
    void remove_from_focus_stack(View* view);

    void begin_interactive(View* view, GrabState::Mode mode, uint32_t edges);
    void process_cursor_motion(Server* server, uint32_t time);
    void process_cursor_move();
    void process_cursor_resize();
    void end_interactive();

    /// Returns the workspace under the cursor.
    std::optional<std::reference_wrapper<Workspace>> get_focused_workspace(Server* server);
};

void init_seat(Server* server, Seat* seat, const char* name);

/**
 * \brief Called when a client wants to set its own mouse cursor image.
 */
void seat_request_cursor_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_SEAT_H_
