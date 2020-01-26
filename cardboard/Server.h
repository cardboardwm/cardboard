#ifndef __CARDBOARD_SERVER_H_
#define __CARDBOARD_SERVER_H_

#include <cstdint>
#include <list>

#include "EventListeners.h"
#include "Keyboard.h"
#include "Output.h"
#include "View.h"

struct Server {
    struct wl_display* wl_display;
    struct wlr_backend* backend;
    struct wlr_renderer* renderer;

    struct wlr_xdg_shell* xdg_shell;
    std::list<View> views;

    struct wlr_cursor* cursor;
    struct wlr_xcursor_manager* cursor_manager;

    struct wlr_seat* seat;
    std::list<Keyboard> keyboards;
    View* grabbed_view;
    double grab_x, grab_y;
    int grab_width, grab_height;
    uint32_t resize_edges;

    struct wlr_output_layout* output_layout;
    std::list<Output> outputs;

    EventListeners listeners;

    void new_keyboard(struct wlr_input_device* device);
    void new_pointer(struct wlr_input_device* device);
    void process_cursor_motion(uint32_t time);
    void focus_view(View* view);

    Server();
    Server(const Server&) = delete;

    bool run();
    void stop();
};

#endif // __CARDBOARD_SERVER_H_
