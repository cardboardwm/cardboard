extern "C" {
#include <wlr/util/log.h>
}

#include "Listener.h"
#include "Server.h"
#include "View.h"
#include "Xwayland.h"

XwaylandView::XwaylandView(Server* server, struct wlr_xwayland_surface* xwayland_surface)
    : View()
    , server(server)
    , xwayland_surface(xwayland_surface)
{
    xwayland_surface->data = this;
}

void XwaylandView::destroy()
{
    server->listeners.clear_listeners(this);
    if (server->seat.grab_state && server->seat.grab_state->view == this) {
        server->seat.end_interactive(server);
    }
    server->views.remove_if([this](const auto x) { return this == x; });
    delete this;
}

void XwaylandView::unmap()
{
    server->unmap_view(this);
}

struct wlr_surface* XwaylandView::get_surface()
{
    return xwayland_surface->surface;
}

bool XwaylandView::get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
{
    double view_x = lx - x;
    double view_y = ly - y;

    double sx_, sy_;
    struct wlr_surface* surface_ = nullptr;
    surface_ = wlr_surface_surface_at(xwayland_surface->surface, view_x, view_y, &sx_, &sy_);

    if (surface_ != nullptr) {
        sx = sx_;
        sy = sy_;
        surface = surface_;
        return true;
    }

    return false;
}

void XwaylandView::resize(int width, int height)
{
    assert(mapped);

    server->get_views_workspace(this)
        .and_then<Output>([](const auto& ws) { return ws.output; })
        .and_then([this, width, height](const auto& output) {
            auto* output_box = wlr_output_layout_get_box(server->output_layout, output.wlr_output);
            wlr_xwayland_surface_configure(
                xwayland_surface, x + output_box->x, y + output_box->y, width, height);
        });
}

void XwaylandView::move(int x_, int y_)
{
    View::move(x_, y_);

    server->get_views_workspace(this)
        .and_then<Output>([](const auto& ws) { return ws.output; })
        .and_then([this](const auto& output) {
            auto* output_box = wlr_output_layout_get_box(server->output_layout, output.wlr_output);
            wlr_xwayland_surface_configure(
                xwayland_surface, x + output_box->x, y + output_box->y, geometry.width, geometry.height);
        });
}

void XwaylandView::prepare(Server* server)
{
    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &xwayland_surface->events.map, xwayland_surface_map_handler },
        { &xwayland_surface->events.unmap, xwayland_surface_unmap_handler },
        { &xwayland_surface->events.destroy, xwayland_surface_destroy_handler },
        { &xwayland_surface->events.request_configure, xwayland_surface_request_configure_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, server, this });
    }
}

void XwaylandView::set_activated(bool activated)
{
    wlr_xwayland_surface_activate(xwayland_surface, activated);
    wlr_xwayland_set_seat(server->xwayland, server->seat.wlr_seat);
}

void XwaylandView::set_fullscreen(bool fullscreen)
{
    wlr_xwayland_surface_set_fullscreen(xwayland_surface, fullscreen);
}

void XwaylandView::for_each_surface(wlr_surface_iterator_func_t iterator, void* data)
{
    wlr_surface_for_each_surface(xwayland_surface->surface, iterator, data);
}

bool XwaylandView::is_transient_for(View* ancestor)
{
    auto* xwayland_ancestor = dynamic_cast<XwaylandView*>(ancestor);
    if (xwayland_ancestor == nullptr) {
        return false;
    }

    auto* surface = xwayland_surface;
    while (surface != nullptr) {
        if (surface->parent == xwayland_ancestor->xwayland_surface) {
            return true;
        }
        surface = surface->parent;
    }

    return false;
}

void XwaylandView::close_popups()
{
    // nothing!
}
void XwaylandView::close()
{
    wlr_xwayland_surface_close(xwayland_surface);
}

void xwayland_surface_map_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* view = get_listener_data<XwaylandView*>(listener);

    if (view->xwayland_surface->override_redirect) {
        auto* xwayland_surface = view->xwayland_surface;
        view->unmap();
        view->destroy();
        auto* xwayland_or_surface = create_xwayland_or_surface(server, xwayland_surface);
        xwayland_or_surface->map(server);
        return;
    }

    view->geometry = (struct wlr_box) {
        .x = 0,
        .y = 0,
        .width = view->xwayland_surface->width,
        .height = view->xwayland_surface->height,
    };

    view->map_unmap_listeners[0] = server->listeners.add_listener(
        &view->xwayland_surface->surface->events.commit,
        Listener { xwayland_surface_commit_handler, server, view });

    view->map_unmap_listeners[1] = server->listeners.add_listener(
        &view->xwayland_surface->events.request_fullscreen,
        Listener { xwayland_surface_request_fullscreen_handler, server, view });

    server->map_view(view);
}

void xwayland_surface_unmap_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* view = get_listener_data<XwaylandView*>(listener);

    assert(view->get_surface() && "Cannot unmap unmapped view");
    view->unmap();
    for (auto* wl_listener : view->map_unmap_listeners) {
        server->listeners.remove_listener(wl_listener);
    }
}

void xwayland_surface_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* view = get_listener_data<XwaylandView*>(listener);

    // unmap handler is guaranteed to be called if the view is mapped

    view->destroy();
}

void xwayland_surface_request_configure_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* view = get_listener_data<XwaylandView*>(listener);
    auto* ev = static_cast<wlr_xwayland_surface_configure_event*>(data);

    if (!view->xwayland_surface->mapped) {
        wlr_xwayland_surface_configure(view->xwayland_surface, ev->x, ev->y, ev->width, ev->height);
        return;
    }

    view->geometry.width = ev->width;
    view->geometry.height = ev->height;
    view->resize(view->geometry.width, view->geometry.height);
    server->get_views_workspace(view).and_then([](auto& ws) { ws.arrange_tiles(); });
}

void xwayland_surface_commit_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* view = get_listener_data<XwaylandView*>(listener);

    auto* xsurface = view->xwayland_surface;
    if (xsurface->x != view->x || xsurface->y != view->y || xsurface->width != view->geometry.width || xsurface->height != view->geometry.height) {
        view->x = xsurface->x;
        view->y = xsurface->y;
        view->geometry.width = xsurface->width;
        view->geometry.height = xsurface->height;

        server->get_views_workspace(view).and_then([](auto& ws) {
            ws.arrange_tiles();
        });
    }
    server->get_views_workspace(view).and_then([view](auto& ws) {
        if (ws.fullscreen_view != OptionalRef(static_cast<View*>(view)) && view->saved_size) {
            wlr_log(WLR_DEBUG, "restoring saved size (%4d, %4d)", view->saved_size->first, view->saved_size->second);
            view->resize(view->saved_size->first, view->saved_size->second);
            view->saved_size = std::nullopt;
        }
    });
}

void xwayland_surface_request_fullscreen_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* view = get_listener_data<XwaylandView*>(listener);

    bool set = view->xwayland_surface->fullscreen;
    server->get_views_workspace(view)
        .and_then<Workspace>([view, set](auto& ws) {
            ws.set_fullscreen_view(set ? view : nullptr);
            return OptionalRef(ws);
        })
        .or_else([view, set]() {
            view->set_fullscreen(set);
            return NullRef<Workspace>;
        });
}

bool XwaylandORSurface::get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
{
    double view_x = lx - xwayland_surface->x;
    double view_y = ly - xwayland_surface->y;

    double sx_, sy_;
    struct wlr_surface* surface_ = nullptr;
    surface_ = wlr_surface_surface_at(xwayland_surface->surface, view_x, view_y, &sx_, &sy_);

    if (surface_ != nullptr) {
        sx = sx_;
        sy = sy_;
        surface = surface_;
        return true;
    }

    return false;
}

void XwaylandORSurface::map(Server* server)
{
    server->xwayland_or_surfaces.push_back(this);
    commit_listener = server->listeners.add_listener(&xwayland_surface->surface->events.commit,
                                                     Listener { xwayland_or_surface_commit_handler, server, this });

    lx = xwayland_surface->x;
    ly = xwayland_surface->y;

    if (wlr_xwayland_or_surface_wants_focus(xwayland_surface)) {
        wlr_xwayland_set_seat(server->xwayland, server->seat.wlr_seat);
        server->seat.focus_surface(server, xwayland_surface->surface);
    }
}

XwaylandORSurface* create_xwayland_or_surface(Server* server, struct wlr_xwayland_surface* xwayland_surface)
{
    wlr_log(WLR_DEBUG, "new xwayland OR surface %d %d", xwayland_surface->x, xwayland_surface->y);
    auto* xwayland_or_surface = new XwaylandORSurface;
    xwayland_or_surface->server = server;
    xwayland_or_surface->xwayland_surface = xwayland_surface;

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &xwayland_or_surface->xwayland_surface->events.map, xwayland_or_surface_map_handler },
        { &xwayland_or_surface->xwayland_surface->events.unmap, xwayland_or_surface_unmap_handler },
        { &xwayland_or_surface->xwayland_surface->events.destroy, xwayland_or_surface_destroy_handler },
        { &xwayland_or_surface->xwayland_surface->events.request_configure, xwayland_or_surface_request_configure_handler }
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(to_add_listener.signal,
                                       Listener { to_add_listener.notify, server, xwayland_or_surface });
    }

    return xwayland_or_surface;
}

void xwayland_or_surface_map_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* xwayland_or_surface = get_listener_data<XwaylandORSurface*>(listener);

    xwayland_or_surface->map(server);
}

void xwayland_or_surface_unmap_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* xwayland_or_surface = get_listener_data<XwaylandORSurface*>(listener);

    server->xwayland_or_surfaces.remove(xwayland_or_surface);
    server->listeners.remove_listener(xwayland_or_surface->commit_listener);
    if (server->seat.wlr_seat->keyboard_state.focused_surface == xwayland_or_surface->xwayland_surface->surface) {
        // restore focus to the last focused view
        if (!server->seat.focus_stack.empty()) {
            server->seat.focus_view(server, nullptr);
            server->seat.focus_view(server, server->seat.focus_stack.front());
        }
    }
}

void xwayland_or_surface_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* server = get_server(listener);
    auto* xwayland_or_surface = get_listener_data<XwaylandORSurface*>(listener);

    server->listeners.clear_listeners(xwayland_or_surface);
    delete xwayland_or_surface;
}

void xwayland_or_surface_request_configure_handler(struct wl_listener* listener, void* data)
{
    auto* xwayland_or_surface = get_listener_data<XwaylandORSurface*>(listener);
    auto* ev = static_cast<struct wlr_xwayland_surface_configure_event*>(data);

    wlr_xwayland_surface_configure(xwayland_or_surface->xwayland_surface, ev->x, ev->y, ev->width, ev->height);
}

void xwayland_or_surface_commit_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    auto* xwayland_or_surface = get_listener_data<XwaylandORSurface*>(listener);

    xwayland_or_surface->lx = xwayland_or_surface->xwayland_surface->x;
    xwayland_or_surface->ly = xwayland_or_surface->xwayland_surface->y;
}
