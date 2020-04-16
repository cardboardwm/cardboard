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
    if (auto ws = server->get_views_workspace(this); ws) {
        return ws.unwrap().output;
    }

    return OptionalRef(static_cast<Output*>(wlr_output_layout_output_at(server->output_layout, x, y)->data));
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

void View::save_size(std::pair<int, int>&& to_save)
{
    assert(!saved_size.has_value());

    saved_size = std::move(to_save);
    wlr_log(WLR_DEBUG, "saved size (%4d, %4d)", saved_size->first, saved_size->second);
}

void create_view(Server* server, View* view_)
{
    server->views.push_back(view_);
    View* view = server->views.back();

    view->prepare(server);
}
