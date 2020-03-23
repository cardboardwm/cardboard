#include <wayland-server.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>
#include <wlr_cpp/util/log.h>

#include <cassert>
#include <limits>

#include "Listener.h"
#include "Server.h"
#include "View.h"

View::View(struct wlr_xdg_surface* xdg_surface)
    : xdg_surface(xdg_surface)
    , geometry { 0, 0, 0, 0 }
    , workspace_id(-1)
    , x(0)
    , y(0)
    , mapped(false)
{
}

void create_view(Server* server, View&& view_)
{
    server->views.push_back(view_);
    View* view = &server->views.back();

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &view->xdg_surface->events.map, xdg_surface_map_handler },
        { &view->xdg_surface->events.unmap, xdg_surface_unmap_handler },
        { &view->xdg_surface->events.destroy, xdg_surface_destroy_handler },
        { &view->xdg_surface->surface->events.commit, xdg_surface_commit_handler },
        { &view->xdg_surface->toplevel->events.request_move, xdg_toplevel_request_move_handler },
        { &view->xdg_surface->toplevel->events.request_resize, xdg_toplevel_request_resize_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, server, view });
    }
}

bool View::get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
{
    double view_x = lx - x;
    double view_y = ly - y;

    double sx_, sy_;
    struct wlr_surface* surface_ = nullptr;
    surface_ = wlr_xdg_surface_surface_at(xdg_surface, view_x, view_y, &sx_, &sy_);

    if (surface_ != nullptr) {
        sx = sx_;
        sy = sy_;
        surface = surface_;
        return true;
    }

    return false;
}

void View::resize(int width, int height)
{
    wlr_xdg_toplevel_set_size(xdg_surface, width, height);
}

void xdg_surface_map_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    view->mapped = true;
    auto* prev_focused = server->get_focused_view();

    wlr_xdg_surface_get_geometry(view->xdg_surface, &view->geometry);

    Workspace& focused_workspace = server->get_focused_workspace();
    focused_workspace.add_view(view, prev_focused);

    server->focus_view(view);
}

void xdg_surface_unmap_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    view->mapped = false;
    if (auto ws = server->get_views_workspace(view)) {
        ws->get().remove_view(view);
    }

    server->hide_view(view);
}
void xdg_surface_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    server->listeners.clear_listeners(view);

    // just in case
    if (auto ws = server->get_views_workspace(view)) {
        ws->get().remove_view(view);
    }

    server->views.remove_if([view](auto& x) { return view == &x; });
}
void xdg_surface_commit_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    struct wlr_box new_geo;
    wlr_xdg_surface_get_geometry(view->xdg_surface, &new_geo);
    if (new_geo.width != view->geometry.width || new_geo.height != view->geometry.height) {
        // the view has set a new size
        wlr_log(WLR_DEBUG, "new size (%3d %3d) -> (%3d %3d)", view->geometry.width, view->geometry.height, new_geo.width, new_geo.height);
        view->geometry = new_geo;

        if (auto workspace = server->get_views_workspace(view)) {
            workspace->get().arrange_tiles();
            workspace->get().fit_view_on_screen(server->get_focused_view());
        }
    }
}
void xdg_toplevel_request_move_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    server->begin_interactive(view, Server::GrabState::Mode::MOVE, 0);
}
void xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    auto* event = static_cast<struct wlr_xdg_toplevel_resize_event*>(data);
    server->begin_interactive(view, Server::GrabState::Mode::RESIZE, event->edges);
}
