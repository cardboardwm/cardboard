extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include <cassert>
#include <limits>

#include "Server.h"
#include "View.h"

SafePointer<Output> View::get_views_output(Server* server)
{
    if (auto ws = server->get_views_workspace(this); ws) {
        return ws.unwrap().output;
    }

    return SafePointer(static_cast<Output*>(wlr_output_layout_output_at(server->output_layout, x, y)->data));
}

void View::change_output(SafePointer<Output> old_output, SafePointer<Output> new_output)
{
    if (old_output && old_output != new_output) {
        wlr_surface_send_leave(get_surface(), old_output.unwrap().wlr_output);
    }

    if (new_output) {
        wlr_surface_send_enter(get_surface(), new_output.unwrap().wlr_output);
    }
}

void create_view(Server* server, View* view_)
{
    server->views.push_back(view_);
    View* view = server->views.back();

    view->prepare(server);
}
