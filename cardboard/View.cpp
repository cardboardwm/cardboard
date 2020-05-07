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

void change_view_workspace(NotNullPointer<Server> server, NotNullPointer<View> view, NotNullPointer<Workspace> new_workspace)
{
    Workspace& workspace = server->workspaces[view->workspace_id];

    workspace.remove_view(static_cast<View*>(view));
    new_workspace->add_view(static_cast<View*>(view), nullptr);
}

void reconfigure_view(NotNullPointer<Server> server, NotNullPointer<View> view, wlr_box logical_geometry)
{
    if(!view->get_views_output(server.get()).has_value()) {
        return;
    }

    view->x = logical_geometry.x;
    view->y = logical_geometry.y;
    view->resize(logical_geometry.width, logical_geometry.height);

    if(
        server->workspaces[view->workspace_id].find_floating(view.get()) !=
        server->workspaces[view->workspace_id].floating_views.end() ) {
        auto* current_output = static_cast<Output*>(wlr_output_layout_output_at(server->output_layout, view->x, view->y)->data);

        if(current_output != server->workspaces[view->workspace_id].output.raw_pointer()) {
            auto workspace = std::find_if(server->workspaces.begin(), server->workspaces.end(), [current_output](auto& w){
               return  w.output.raw_pointer() == current_output;
            });

            change_view_workspace(server, view, &(*workspace));
        }
    }
}
