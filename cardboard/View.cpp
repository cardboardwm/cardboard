#include <wayland-server.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>

#include "Server.h"
#include "View.h"
#include "Listener.h"

View::View(struct wlr_xdg_surface* xdg_surface):
    xdg_surface(xdg_surface)
    , mapped(false)
    , x(0)
    , y(0)
{
}

void create_view(Server* server, View&& view_)
{
    server->views.push_back(std::move(view_));
    View* view = &server->views.back();

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        {&view->xdg_surface->events.map, xdg_surface_map_handler},
        {&view->xdg_surface->events.unmap, xdg_surface_unmap_handler},
        {&view->xdg_surface->events.destroy, xdg_surface_destroy_handler},
        {&view->xdg_surface->toplevel->events.request_move, xdg_toplevel_request_move_handler},
        {&view->xdg_surface->toplevel->events.request_resize, xdg_toplevel_request_resize_handler},
    };

    for(const auto& to_add_listener: to_add_listeners)
    {
        server->listeners.add_listener(
            to_add_listener.signal,
            Listener{to_add_listener.notify, this, view}
        );
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

void xdg_surface_map_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    view->mapped = true;
    server->focus_view(view, view->xdg_surface->surface);
}
void xdg_surface_unmap_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);

    view->mapped = false;
}
void xdg_surface_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    server->views.erase(view->link);
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
