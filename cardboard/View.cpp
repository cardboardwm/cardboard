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

void View::xdg_surface_map_handler(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, map);
    view->mapped = true;
    (void)(data);
}
void View::xdg_surface_unmap_handler(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, unmap);
    view->mapped = false;
    (void)(data);
}
void View::xdg_surface_destroy_handler(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, destroy);
    view->server->views.erase(view->link);
    (void)(data);
}
void View::xdg_toplevel_request_move_handler(struct wl_listener* listener, void* data)
{
    (void)(listener);
    (void)(data);
}
void View::xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data)
{
    (void)(listener);
    (void)(data);
}
