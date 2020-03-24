#include <wlr_cpp/util/log.h>

#include "Server.h"
#include "XDGView.h"

XDGView::XDGView(struct wlr_xdg_surface* xdg_surface)
    : View()
    , xdg_surface(xdg_surface)
{
}

struct wlr_surface* XDGView::get_surface()
{
    return xdg_surface->surface;
}

bool XDGView::get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
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

void XDGView::resize(int width, int height)
{
    wlr_xdg_toplevel_set_size(xdg_surface, width, height);
}

void XDGView::prepare(Server* server)
{
    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &xdg_surface->events.map, xdg_surface_map_handler },
        { &xdg_surface->events.unmap, xdg_surface_unmap_handler },
        { &xdg_surface->events.destroy, xdg_surface_destroy_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, server, this });
    }
}

void XDGView::set_activated(bool activated)
{
    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }
    wlr_xdg_toplevel_set_activated(xdg_surface, activated);
}

void XDGView::for_each_surface(wlr_surface_iterator_func_t iterator, void* data)
{
    wlr_xdg_surface_for_each_surface(xdg_surface, iterator, data);
}

void xdg_surface_map_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    wlr_xdg_surface_get_geometry(view->xdg_surface, &view->geometry);
    if (!view->geometry.width && !view->geometry.height) {
        view->geometry.width = view->xdg_surface->surface->current.width;
        view->geometry.height = view->xdg_surface->surface->current.height;
    }

    server->map_view(view);

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &view->xdg_surface->surface->events.commit, xdg_surface_commit_handler },
        { &view->xdg_surface->toplevel->events.request_move, xdg_toplevel_request_move_handler },
        { &view->xdg_surface->toplevel->events.request_resize, xdg_toplevel_request_resize_handler }
    };

    for (unsigned long i = 0; i < sizeof(to_add_listeners) / sizeof(*to_add_listeners); i++) {
        view->map_unmap_listeners[i] = server->listeners.add_listener(to_add_listeners[i].signal,
                                                                      Listener { to_add_listeners[i].notify, server, view });
    }
}

void xdg_surface_unmap_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    server->unmap_view(view);

    for (auto* listener : view->map_unmap_listeners) {
        server->listeners.remove_listener(listener);
    }
}
void xdg_surface_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    server->listeners.clear_listeners(view);

    // just in case
    if (auto ws = server->get_views_workspace(view)) {
        ws->get().remove_view(view);
    }

    server->views.remove_if([view](const auto x) { return view == x; });
    delete view;
}
void xdg_surface_commit_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

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
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    server->begin_interactive(view, Server::GrabState::Mode::MOVE, 0);
}
void xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    auto* event = static_cast<struct wlr_xdg_toplevel_resize_event*>(data);
    server->begin_interactive(view, Server::GrabState::Mode::RESIZE, event->edges);
}
