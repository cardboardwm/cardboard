#ifndef CARDBOARD_SERVER_H_INCLUDED
#define CARDBOARD_SERVER_H_INCLUDED

#include "BuildConfig.h"

extern "C" {
#include <wayland-server.h>
#define static
#include <wlr/types/wlr_compositor.h>
#undef static
#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
}

#include <wlr_cpp_fixes/types/wlr_layer_shell_v1.h>

#include <sys/un.h>

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Config.h"
#include "IPC.h"
#include "Keyboard.h"
#include "Layers.h"
#include "Listener.h"
#include "NotNull.h"
#include "OptionalRef.h"
#include "Output.h"
#include "OutputManager.h"
#include "Seat.h"
#include "View.h"
#include "ViewManager.h"
#include "Workspace.h"

#if HAVE_XWAYLAND
#include <wlr_cpp_fixes/xwayland.h>
#endif

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
    struct wlr_compositor* compositor;
    struct wlr_backend* backend;
    struct wlr_renderer* renderer;

    IPCInstance ipc;
    /// Event loop object for integrating IPC events with the rest of Wayland's event system.
    wl_event_loop* event_loop;
    /// IPC event source for Wayland' event loop mechanism.
    wl_event_source* ipc_event_source;

    std::string config_path;

    struct wlr_xdg_shell* xdg_shell;
    struct wlr_layer_shell_v1* layer_shell;
    struct wlr_xwayland* xwayland;

#if HAVE_XWAYLAND
    std::list<std::unique_ptr<XwaylandORSurface>> xwayland_or_surfaces;
#endif
    LayerArray layers;

    std::vector<Workspace> workspaces;

    OutputManager output_manager;
    ViewManager view_manager;

    ListenerList listeners;
    KeybindingsConfig keybindings_config;
    Config config;

    Seat seat;

    int exit_code = EXIT_SUCCESS;

    Server()
    {
    }
    Server(const Server&) = delete;

    /// Initializes the Wayland compositor.
    bool init();
    /// Creates socket file
    bool init_ipc();
#if HAVE_XWAYLAND
    void init_xwayland();
#endif
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

    /// Creates a new workspace, without any assigned output.
    Workspace& create_workspace();

    /// Runs the event loop.
    bool run();
    /// Stops the engines.
    void stop();
    /// Stops the event loop. Runs before Server::stop.
    void teardown(int code);

private:
    /**
    * \brief Called when a new \c xdg_surface is created by a client.
    *
    * An \c xdg-surface is a type of surface exposed by the \c xdg-shell.
    * This handler creates a View based on it.
    */
    static void new_xdg_surface_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when a new \c layer_surface is created by a client.
    *
    * A \c layer_surface is a type of surface exposed by the \c layer-shell.
    * This handler does NOT create Views based on it.
    */
    static void new_layer_surface_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Called when a new \c xwayland_surface is spawned by an X client.
    *
    * An \c xwayland_surface is a type of surface exposed by the xwayland wlroots system.
    */
    static void new_xwayland_surface_handler(struct wl_listener* listener, void* data);
};

#endif // CARDBOARD_SERVER_H_INCLUDED
