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
#include "Tiling.h"
#include "View.h"

constexpr std::string_view CARDBOARD_NAME = "cardboard";
constexpr std::string_view CONFIG_NAME = "cardboardrc";
constexpr std::string_view CONFIG_HOME_ENV = "XDG_CONFIG_HOME";

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

    sockaddr_un ipc_sock_address;
    int ipc_socket_fd = -1;
    wl_event_loop* event_loop;
    wl_event_source* ipc_event_source;

    std::string config_path;

    struct wlr_xdg_shell* xdg_shell;
    // ordered in stacking order
    std::list<View> views; // TODO: check if View's statefulness is needed
        // change to std::vector if it's not needed +
        // change in ListenerData from View* to View
    TilingSequence tiles;
    std::list<View*> focused_views;

    struct wlr_cursor* cursor;
    struct wlr_xcursor_manager* cursor_manager;

    struct wlr_seat* seat;
    std::vector<wlr_input_device*> keyboards;
    KeybindingsConfig keybindingsConfig;
    std::optional<GrabState> grab_state;

    struct wlr_output_layout* output_layout;
    std::vector<wlr_output*> outputs;

    ListenerList listeners;

    int exit_code = EXIT_SUCCESS;

    Server() {}
    Server(const Server&) = delete;

    bool init();
    bool init_ipc1();
    void init_ipc2();
    bool load_settings();

    void new_keyboard(struct wlr_input_device* device);
    void new_pointer(struct wlr_input_device* device);

    void process_cursor_motion(uint32_t time);
    void process_cursor_move();
    void process_cursor_resize();

    void focus_view(View* view);
    // Focus the offset-nth tiled window to the right (or to the left if negative) of the currently
    // focused view.
    void focus_by_offset(int offset);

    // Returns the xdg surface leaf of the first view under the cursor
    View* get_surface_under_cursor(double rx, double ry, struct wlr_surface*& surface, double& sx, double& sy);
    View* get_focused_view();

    void begin_interactive(View* view, GrabState::Mode mode, uint32_t edges);

    bool run();
    void stop();
    void teardown(int code);
};

#endif // __CARDBOARD_SERVER_H_
