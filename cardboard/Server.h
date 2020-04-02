#ifndef __CARDBOARD_SERVER_H_
#define __CARDBOARD_SERVER_H_

#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_compositor.h>
#include <wlr_cpp/types/wlr_cursor.h>
#include <wlr_cpp/types/wlr_data_device.h>
#include <wlr_cpp/types/wlr_output_layout.h>
#include <wlr_cpp/types/wlr_xcursor_manager.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>

#include <layer_shell_v1.h>

#include <sys/un.h>

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "EventListeners.h"
#include "IPC.h"
#include "Keyboard.h"
#include "Layers.h"
#include "Listener.h"
#include "Output.h"
#include "Seat.h"
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
    struct wlr_layer_shell_v1* layer_shell;

    /// Holds the active views, ordered by the stacking order.
    std::list<View*> views; // TODO: check if View's statefulness is needed
        // change to std::vector if it's not needed +
        // change in ListenerData from View* to View

    std::vector<Workspace> workspaces;

    struct wlr_output_layout* output_layout;
    std::list<Output> outputs;

    ListenerList listeners;
    KeybindingsConfig keybindings_config;

    Seat seat;

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
    /// Common mapping procedure for views regardless of their underlying shell.
    void map_view(View* view);
    /// Common unmapping procedure for views regardless of their underlying shell.
    void unmap_view(View* view);
    /// Puts the \a view on top.
    void move_view_to_front(View* view);

    /// Returns the workspace in which the given \a view resides, if any.
    std::optional<std::reference_wrapper<Workspace>> get_views_workspace(View* view);
    /// Creates a new workspace, without any assigned output.
    Workspace& create_workspace();

    /// Runs the event loop.
    bool run();
    /// Stops the engines.
    void stop();
    /// Stops the event loop. Runs before Server::stop.
    void teardown(int code);
};

#endif // __CARDBOARD_SERVER_H_
