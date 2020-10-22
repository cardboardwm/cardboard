// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_SEAT_H_INCLUDED
#define CARDBOARD_SEAT_H_INCLUDED

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_seat.h>
}

#include <wlr_cpp_fixes/types/wlr_layer_shell_v1.h>

#include <functional>
#include <list>
#include <optional>
#include <variant>
#include <vector>

#include "Cursor.h"
#include "Keyboard.h"
#include "NotNull.h"
#include "OptionalRef.h"
#include "View.h"

struct Server;
struct OutputManager;

constexpr const char* DEFAULT_SEAT = "seat0";
const int WORKSPACE_SCROLL_FINGERS = 3;
const double WORKSPACE_SCROLL_SENSITIVITY = 2.0; ///< sensitivity multiplier
const double WORKSPACE_SCROLL_FRICTION = 0.9; ///< friction multiplier
const int WORKSPACE_SWITCH_FINGERS = 4;

struct Seat {
    struct GrabState {
        struct Move {
            NotNullPointer<View> view;
            double lx, ly;
            int view_x, view_y;
        };
        struct Resize {
            NotNullPointer<View> view;
            double lx, ly;
            struct wlr_box geometry;
            uint32_t resize_edges;
            NotNullPointer<Workspace> workspace;
            int scroll_x;
            int view_x, view_y;
            bool old_suspend_animations;
        };
        struct WorkspaceScroll {
            NotNullPointer<Workspace> workspace;
            OptionalRef<View> dominant_view;
            double speed; ///< scrolling speed
            double delta_since_update; ///< how much the fingers moved since the last update
            double scroll_x; ///< the scroll of the workspace stored as a double
            bool ready; ///< set to true after the fingers move for the first time
            bool wants_to_stop; ///< set to true after lifting the fingers off the touchpad
        };
        struct WorkspaceSwitch {
            NotNullPointer<Workspace> workspace;
            double direction; ///negative for "upper" workspaces, positive for below workspaces
        };
        std::variant<Move, Resize, WorkspaceScroll, WorkspaceSwitch> grab_data;
    };

    SeatCursor cursor;
    std::optional<GrabState> grab_state;

    struct wlr_seat* wlr_seat;
    struct wlr_input_inhibit_manager* inhibit_manager;

    std::list<Keyboard> keyboards;
    std::list<NotNullPointer<View>> focus_stack; ///< Views ordered by the time they were focused, from most recent.

    std::optional<struct wlr_layer_surface_v1*> focused_layer;
    std::optional<struct wl_client*> exclusive_client;

    void register_handlers(Server& server, struct wl_signal* new_input);

    /// Returns the currently focused View. It is defined as the View currently holding keyboard focus.
    OptionalRef<View> get_focused_view();
    /// Hides the \a view from the screen without unmapping. Happens when a Workspace is deactivated.
    void hide_view(Server& server, View& view);
    /// Gives keyboard focus to a plain surface (OR xwayland usually)
    void focus_surface(struct wlr_surface* surface);
    /**
     * \brief Sets the focus state on \a view. Auto-scrolls the Workspace if it's tiled.
     *
     * If \a view is null, the previously focused view will be unfocused and no other view will be focused.
     */
    void focus_view(Server& server, OptionalRef<View> view, bool condense_workspace = false);
    /// Marks the layer as receiving keyboard focus from this seat.
    void focus_layer(Server& server, struct wlr_layer_surface_v1* layer);
    /**
     * \brief Focus the most recently focused view on \a column.
     *
     * \param column - must be from this workspace
     */
    void focus_column(Server& server, Workspace::Column& column);
    /// Removes the \a view from the focus stack.
    void remove_from_focus_stack(View& view);

    void begin_move(Server& server, View& view);
    void begin_resize(Server& server, View& view, uint32_t edges);
    void begin_workspace_scroll(Server& server, Workspace& workspace);
    void process_cursor_motion(Server& server, uint32_t time = 0);
    void process_cursor_move(Server&, GrabState::Move move_data);
    void process_cursor_resize(Server&, GrabState::Resize resize_data);
    void process_swipe_begin(Server& server, uint32_t fingers);
    void process_swipe_update(Server& server, uint32_t fingers, double dx, double dy);
    void process_swipe_end(Server& server);
    void end_interactive(Server& server);
    void end_touchpad_swipe(Server& server);

    /// Updates the scroll of the workspace during three-finger swipe, taking in account speed and friction.
    void update_swipe(Server& server);

    /// Returns true if the \a view is currently in a grab operation.
    bool is_grabbing(View& view);

    /// Returns the workspace under the cursor.
    OptionalRef<Workspace> get_focused_workspace(Server& server);

    /// Moves the focus to a different workspace, if the workspace is already on a monitor, it focuses that monitor
    void focus(Server& server, Workspace& workspace); // TODO: yikes, passing Server*

    /// Considers a \a client as exclusive. Only the surfaces of the \a client will get input events.
    void set_exclusive_client(Server& server, struct wl_client* client);
    /**
     * \brief Returns true if input events can be delivered to \a surface.
     *
     * Input events can be delivered freely when there is no exclusive client. If there is
     * an exclusive client set, input may be allowed only if the surface belongs to that client.
     */
    bool is_input_allowed(struct wlr_surface* surface);

    /// Returns true if the mod keys is pressed on any of this seat's keyboards.
    bool is_mod_pressed(uint32_t mods);

    /// Sets keyboard focus on a \a surface.
    void keyboard_notify_enter(struct wlr_surface* surface);

public:
    /**
    * \brief Called when an input device (keyboard, mouse, touchscreen, tablet) is attached.
    *
    * The device is registered within the compositor accordingly.
    */
    static void new_input_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Activates the inhibition of input events for the requester.
    *
    * Per the wlr-input-inhibitor protocol.
    */
    static void activate_inhibit_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Deactivates the inhibiton of input events for the requester.
    *
    * Per the wlr-input-inhibitor protocol.
    */
    static void deactivate_inhibit_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when the cursor (mouse) is moved.
    *
    * In some cases, the mouse might "jump" to a position instead of moving by a certain delta.
    * This happens with the X11 backend for example. This handler only handles moves by a delta.
    *
    * \sa cursor_motion_absolute_handler for when the cursor "jumps".
    */
    static void cursor_motion_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when the cursor "jumps" or "warps" to an absolute position on the screen.
    *
    * \sa cursor_motion_handler for cursor moves by a delta.
    */
    static void cursor_motion_absolute_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Handles mouse button clicks.
    *
    * Besides transmitting clicks to the clients (by the means of Server::seat),
    * it also focuses the window under the cursor, with its side effects, such as auto-scrolling
    * the viewport of the Workspace the View under the cursor is in.
    */
    static void cursor_button_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Handles scrolling.
    */
    static void cursor_axis_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Handles pointer frame events.
    *
    * A frame event is sent after regular pointer events to group multiple events together.
    * For instance, two axis events may happend at the same time, in which case a frame event
    * won't be sent in between.
    */
    static void cursor_frame_handler(struct wl_listener* listener, void* data);

    /**
     * \brief Called when the user starts a swipe on the touchpad (more than one finger).
     */
    static void cursor_swipe_begin_handler(struct wl_listener* listener, void* data);

    /**
     * \brief Called when the user moved their fingers during a swipe on the touchpad.
     */
    static void cursor_swipe_update_handler(struct wl_listener* listener, void* data);

    /**
     * \brief Called after the user lifted all their fingers, eding the swipe.
     */
    static void cursor_swipe_end_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when a client wants to set its own mouse cursor image.
    */
    static void request_cursor_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when a client puts something in the clipboard (Ctrl-C)
    */
    static void request_selection_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when a client puts something in the primary selection (selects some text).
    */
    static void request_primary_selection_handler(struct wl_listener* listener, void* data);
};

void init_seat(Server& server, Seat& seat, const char* name);

#endif // CARDBOARD_SEAT_H_INCLUDED
