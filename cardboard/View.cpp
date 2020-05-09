extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include <cassert>
#include <limits>
#include <utility>

#include "Server.h"
#include "View.h"

OptionalRef<Output> View::get_views_output(Server* server)
{
    if (workspace_id < 0) {
        return NullRef<Output>;
    }
    return server->get_views_workspace(this).output;
}

void View::change_output(OptionalRef<Output> old_output, OptionalRef<Output> new_output)
{
    if (old_output && old_output != new_output) {
        wlr_surface_send_leave(get_surface(), old_output.unwrap().wlr_output);
    }

    if (new_output) {
        wlr_surface_send_enter(get_surface(), new_output.unwrap().wlr_output);
    }
}

void View::save_state(ExpansionSavedState to_save)
{
    assert(!saved_state.has_value() && expansion_state == ExpansionState::NORMAL);

    saved_state = to_save;
    wlr_log(WLR_DEBUG, "saved size (%4d, %4d)", saved_state->width, saved_state->height);
}

void View::recover()
{
    if (expansion_state == ExpansionState::RECOVERING) {
        expansion_state = ExpansionState::NORMAL;
    }
}

void View::move(int x_, int y_)
{
    x = x_;
    y = y_;
}

void create_view(Server* server, View* view_)
{
    server->views.push_back(view_);
    View* view = server->views.back();

    view->prepare(server);
}
