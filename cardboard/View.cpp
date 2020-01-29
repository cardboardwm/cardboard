#include <wayland-server.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>

#include "Server.h"
#include "View.h"

View::View(Server* server, struct wlr_xdg_surface* xdg_surface)
    : server(server)
    , xdg_surface(xdg_surface)
    , mapped(false)
    , x(0)
    , y(0)
{
    map.notify = View::xdg_surface_map_handler;
    unmap.notify = View::xdg_surface_unmap_handler;
    destroy.notify = View::xdg_surface_destroy_handler;
    wl_signal_add(&xdg_surface->events.map, &map);
    wl_signal_add(&xdg_surface->events.unmap, &unmap);
    wl_signal_add(&xdg_surface->events.destroy, &destroy);

    struct wlr_xdg_toplevel* toplevel = xdg_surface->toplevel;
    request_move.notify = View::xdg_toplevel_request_move_handler;
    wl_signal_add(&toplevel->events.request_move, &request_move);
    request_resize.notify = View::xdg_toplevel_request_resize_handler;
    wl_signal_add(&toplevel->events.request_resize, &request_resize);
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

void View::xdg_surface_map_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = wl_container_of(listener, view, map);
    view->mapped = true;
    view->server->focus_view(view, view->xdg_surface->surface);
}
void View::xdg_surface_unmap_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = wl_container_of(listener, view, unmap);
    view->mapped = false;
}
void View::xdg_surface_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = wl_container_of(listener, view, destroy);
    view->server->views.erase(view->link);
}
void View::xdg_toplevel_request_move_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = wl_container_of(listener, view, request_move);
    view->server->begin_interactive(view, Server::GrabState::Mode::MOVE, 0);
}
void View::xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, request_resize);
    auto* event = static_cast<struct wlr_xdg_toplevel_resize_event*>(data);
    view->server->begin_interactive(view, Server::GrabState::Mode::RESIZE, event->edges);
}
