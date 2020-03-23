#ifndef __CARDBOARD_SERVER_H_
#define __CARDBOARD_SERVER_H_

#include <sys/un.h>

#include <cstdint>
#include <list>
#include <optional>
#include <string>

#include "EventListeners.h"
#include "IPC.h"
#include "Keyboard.h"
#include "Listener.h"
#include "Output.h"
#include "View.h"
#include "Workspace.h"

const std::string_view CARDBOARD_NAME = "cardboard"; ///< The name of the compositor.
const std::string_view CONFIG_NAME = "cardboardrc"; ///< The name of the config script.
/// The environment variable for the standard config directory.
const std::string_view CONFIG_HOME_ENV = "XDG_CONFIG_HOME";

const int WORKSPACE_NR = 4; ///< Default number of pre-initialized workspaces.

/**
 * \brief Holds all the information of the currently running compositor.
 *
 * Its methods also implement most of the basic functionality not related to other structures.
 */
struct Server {
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

    struct wl_display* wl_display;
    struct wlr_backend* backend;
    struct wlr_renderer* renderer;

    /// The address of the UNIX domain socket used for IPC.
    sockaddr_un ipc_sock_address;
    /// File descriptor for the IPC socket.
    int ipc_socket_fd = -1;
    /// Event loop object for integrating IPC events with the rest of Wayland's event system.
    wl_event_loop* event_loop;
    /// IPC event source for Wayland' event loop mechanism.
    wl_event_source* ipc_event_source;

    std::string config_path;

    struct wlr_xdg_shell* xdg_shell;
    /// Holds the active views, ordered by the stacking order.
    std::list<View> views; // TODO: check if View's statefulness is needed
        // change to std::vector if it's not needed +
        // change in ListenerData from View* to View
    std::list<View*> focused_views; ///< Views ordered by the time they were focused, from most recent.

    std::vector<Workspace> workspaces;

    struct wlr_cursor* cursor;
    struct wlr_xcursor_manager* cursor_manager;

    struct wlr_seat* seat;
    std::vector<wlr_input_device*> keyboards;
    KeybindingsConfig keybindings_config;
    std::optional<GrabState> grab_state;

    struct wlr_output_layout* output_layout;
    std::vector<wlr_output_layout_output*> outputs;

    ListenerList listeners;

    int exit_code = EXIT_SUCCESS;

    Server() {}
    Server(const Server&) = delete;

    /// Initializes the Wayland compositor.
    bool init();
    /// Connects to the IPC socket before Wayland is initialized.
    bool init_ipc1();
    /// Wires up the IPC socket file descriptor to Wayland's event loop.
    void init_ipc2();
    /// Runs the config script in background. Executed before Server::init_ipc2.
    bool load_settings();

    /// Sets up a new keyboard device, with events and keymap.
    void new_keyboard(struct wlr_input_device* device);
    /// Attaches a new pointer device (e.g. mouse) to the cursor.
    void new_pointer(struct wlr_input_device* device);

    void process_cursor_motion(uint32_t time);
    void process_cursor_move();
    void process_cursor_resize();

    /// Sets the focus state on \a view. Auto-scrolls the Workspace if it's tiled.
    void focus_view(View* view);
    /**
     * \brief Focus the <em>offset</em>-nth tiled window to the right (or to the left if negative) of the currently
     * focused view.
     */
    void focus_by_offset(int offset);
    /// Hides the window from the screen but without unmapping. Happends when a Workspace is deactivated.
    void hide_view(View* view);

    /**
     * \brief Returns the xdg surface leaf of the first view under the cursor.
     *
     * \a rx and \a ry are root coordinates. That is, coordinates relative to the imaginary plane of all surfaces.
     *
     * \param[out] surface The \c wlr_surface found under the cursor.
     * \param[out] sx The x coordinate of the found surface in root coordinates.
     * \param[out] sy The y coordinate of the found surface in root coordinates.
     */
    View* get_surface_under_cursor(double rx, double ry, struct wlr_surface*& surface, double& sx, double& sy);
    /// Returns the currently focused View. It is defined as the View currently holding keyboard focus.
    View* get_focused_view();

    /// Returns the workspace in which the given \a view resides, if any.
    std::optional<std::reference_wrapper<Workspace>> get_views_workspace(View* view);
    /// Returns the workspace under the cursor.
    Workspace& get_focused_workspace();
    /// Creates a new workspace, without any assigned output.
    Workspace& create_workspace();

    void begin_interactive(View* view, GrabState::Mode mode, uint32_t edges);

    /// Runs the event loop.
    bool run();
    /// Stops the engines.
    void stop();
    /// Stops the event loop. Runs before Server::stop.
    void teardown(int code);
};

#endif // __CARDBOARD_SERVER_H_
