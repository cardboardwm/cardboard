#ifndef UTIL_SEAT_H
#define UTIL_SEAT_H

#include <optional>

#include "Keyboard.h"
#include "View.h"

enum class CursorMode
{
    PASSTHROUGH,
    MOVE,
    RESIZE
};

struct Seat
{
    wlr_seat* seat;

    ListenerId cursor_motion;
    ListenerId cursor_motion_absolute;
    ListenerId cursor_button;
    ListenerId cursor_axis;
    ListenerId cursor_frame;

    ListenerId new_input;
    ListenerId request_cursor;
    ListenerId request_set_selection;

    std::vector<Keyboard> keyboards;
    CursorMode cursor_mode;

    std::vector<ViewId> focus_stack;

    struct GrabState
    {
        ViewId view;
        double x, y;
        wlr_box geometric_box;
        uint32_t resize_edges;
    };

    std::optional<GrabState> grab_state;
};

Seat create_seat(Server* server);
void destroy(Server* server, const Seat& seat);

#endif //UTIL_SEAT_H
