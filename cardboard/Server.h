#ifndef __CARDBOARD_SERVER_H_
#define __CARDBOARD_SERVER_H_

#include <cstdint>
#include <list>
#include <optional>

#include "EventListeners.h"
#include "Keyboard.h"
#include "Listener.h"
#include "Output.h"
#include "Tiling.h"
#include "View.h"

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

    struct wlr_xdg_shell* xdg_shell;
    std::list<View> views; // TODO: check if View's statefulness is needed
        // change to std::vector if it's not needed +
        // change in ListenerData from View* to View
    TilingSequence tiles;

    struct wlr_cursor* cursor;
    struct wlr_xcursor_manager* cursor_manager;

    struct wlr_seat* seat;
    std::vector<wlr_input_device*> keyboards;
    KeybindingsConfig keybindingsConfig;
    std::optional<GrabState> grab_state;

    struct wlr_output_layout* output_layout;
    std::vector<wlr_output*> outputs;

    ListenerList listeners;

    Server();
    Server(const Server&) = delete;

    bool run();
    void stop();

    void new_keyboard(struct wlr_input_device* device);
    void new_pointer(struct wlr_input_device* device);
    void process_cursor_motion(uint32_t time);
    void process_cursor_move();
    void process_cursor_resize();
    void focus_view(View* view);
    // Returns the xdg surface leaf of the first view under the cursor
    View* get_surface_under_cursor(double rx, double ry, struct wlr_surface*& surface, double& sx, double& sy);
    View* get_focused_view();
    void begin_interactive(View* view, GrabState::Mode mode, uint32_t edges);
};

#endif // __CARDBOARD_SERVER_H_
