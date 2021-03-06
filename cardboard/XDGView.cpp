// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
extern "C" {
#include <wlr/util/log.h>
}

#include <cstring>

#include "Helpers.h"
#include "Server.h"
#include "XDGView.h"

XDGView::XDGView(struct wlr_xdg_surface* xdg_surface)
    : View()
    , xdg_surface(xdg_surface)
{
    xdg_surface->data = this;
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
    View::resize(width, height);
    wlr_xdg_toplevel_set_size(xdg_surface, width, height);
}

void XDGView::prepare(Server& server)
{
    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &xdg_surface->events.map, XDGView::surface_map_handler },
        { &xdg_surface->events.unmap, XDGView::surface_unmap_handler },
        { &xdg_surface->events.destroy, XDGView::surface_destroy_handler },

        { &xdg_surface->events.new_popup, XDGView::surface_new_popup_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server.listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, &server, this });
    }
}

void XDGView::set_activated(bool activated)
{
    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }
    wlr_xdg_toplevel_set_activated(xdg_surface, activated);
}

void XDGView::set_fullscreen(bool fullscreen)
{
    wlr_xdg_toplevel_set_fullscreen(xdg_surface, fullscreen);
}

void XDGView::for_each_surface(wlr_surface_iterator_func_t iterator, void* data)
{
    wlr_xdg_surface_for_each_surface(xdg_surface, iterator, data);
}

bool XDGView::is_transient_for(View& ancestor)
{
    auto* xdg_ancestor = dynamic_cast<XDGView*>(&ancestor);
    if (xdg_ancestor == nullptr) {
        return false;
    }

    auto* surface = xdg_surface;
    while (surface && surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        if (surface->toplevel->parent == xdg_ancestor->xdg_surface) {
            return true;
        }
        surface = surface->toplevel->parent;
    }

    return false;
}

void XDGView::close_popups()
{
    struct wlr_xdg_popup* popup;
    struct wlr_xdg_popup* tmp;
    wl_list_for_each_safe(popup, tmp, &xdg_surface->popups, link)
    {
        wlr_xdg_popup_destroy(popup->base);
    }
}
void XDGView::close()
{
    wlr_xdg_toplevel_send_close(xdg_surface);
}

XDGPopup::XDGPopup(struct wlr_xdg_popup* wlr_popup, NotNullPointer<XDGView> parent)
    : wlr_popup(wlr_popup)
    , parent(parent)
{
}

void XDGPopup::unconstrain(Server& server)
{
    const auto output = server.output_manager->get_view_workspace(*parent).output;
    if (!output) {
        return;
    }

    const struct wlr_box* output_box = server.output_manager->get_output_box(output.unwrap());

    // the output box expressed in the coordinate system of the
    // toplevel parent of the popup
    struct wlr_box output_toplevel_sx_box = {
        .x = output_box->x - parent->x,
        .y = output_box->y - parent->y,
        .width = output_box->width,
        .height = output_box->height,
    };

    wlr_xdg_popup_unconstrain_from_box(wlr_popup, &output_toplevel_sx_box);
}

static void create_xdg_popup(Server& server, struct wlr_xdg_popup* wlr_popup, NotNullPointer<XDGView> parent)
{
    auto* popup = new XDGPopup { wlr_popup, parent };

    register_handlers(server,
                      popup,
                      {
                          { &popup->wlr_popup->base->events.destroy, &XDGPopup::destroy_handler },
                          { &popup->wlr_popup->base->events.new_popup, &XDGPopup::new_popup_handler },
                          { &popup->wlr_popup->base->events.map, &XDGPopup::map_handler },
                      });

    popup->unconstrain(server);
}

void XDGView::surface_map_handler(struct wl_listener* listener, void*)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    wlr_xdg_surface_get_geometry(view->xdg_surface, &view->geometry);
    if (!view->geometry.width && !view->geometry.height) {
        view->geometry.width = view->xdg_surface->surface->current.width;
        view->geometry.height = view->xdg_surface->surface->current.height;
    }

    if (view->new_view) {
        view->previous_size = { view->geometry.width, view->geometry.height };
        view->new_view = false;
    }

    server->surface_manager.map_view(*server, *view);

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &view->xdg_surface->surface->events.commit, XDGView::surface_commit_handler },
        { &view->xdg_surface->toplevel->events.request_move, XDGView::toplevel_request_move_handler },
        { &view->xdg_surface->toplevel->events.request_resize, XDGView::toplevel_request_resize_handler },
        { &view->xdg_surface->toplevel->events.request_fullscreen, XDGView::toplevel_request_fullscreen_handler },
    };

    for (unsigned long i = 0; i < sizeof(to_add_listeners) / sizeof(*to_add_listeners); i++) {
        view->map_unmap_listeners[i] = server->listeners.add_listener(to_add_listeners[i].signal,
                                                                      Listener { to_add_listeners[i].notify, server, view });
    }
}

void XDGView::surface_unmap_handler(struct wl_listener* listener, void*)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    server->surface_manager.unmap_view(*server, *view);

    for (auto* listener : view->map_unmap_listeners) {
        server->listeners.remove_listener(listener);
    }
}

void XDGView::surface_destroy_handler(struct wl_listener* listener, void*)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    server->listeners.clear_listeners(view);

    // unmap handler is guaranteed to be called if the view is mapped

    if (server->seat.is_grabbing(*view)) {
        server->seat.end_interactive(*server);
    }
    server->surface_manager.remove_view(*server->view_animation, *view);
}

void XDGView::surface_commit_handler(struct wl_listener* listener, void*)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    struct wlr_box new_geo;
    wlr_xdg_surface_get_geometry(view->xdg_surface, &new_geo);
    auto& ws = server->output_manager->get_view_workspace(*view);
    if (memcmp(&new_geo, &view->geometry, sizeof(struct wlr_box)) != 0) {
        // the view has set a new size
        wlr_log(WLR_DEBUG, "new size (%3d %3d) -> (%3d %3d)", view->geometry.width, view->geometry.height, new_geo.width, new_geo.height);
        view->geometry = new_geo;
        view->recover();

        ws.arrange_workspace(*(server->output_manager));
    }
    server->output_manager->set_dirty();
}

void XDGView::surface_new_popup_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* view = get_listener_data<XDGView*>(listener);
    auto* wlr_popup = static_cast<struct wlr_xdg_popup*>(data);

    create_xdg_popup(*server, wlr_popup, view);
}

void XDGView::toplevel_request_move_handler(struct wl_listener* listener, void*)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    server->seat.begin_move(*server, *view);
}
void XDGView::toplevel_request_resize_handler(struct wl_listener* listener, void* data)
{
    auto* view = get_listener_data<XDGView*>(listener);
    auto* server = get_server(listener);

    auto* event = static_cast<struct wlr_xdg_toplevel_resize_event*>(data);
    server->seat.begin_resize(*server, *view, event->edges);
}

void XDGView::toplevel_request_fullscreen_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* view = get_listener_data<XDGView*>(listener);
    auto* event = static_cast<struct wlr_xdg_toplevel_set_fullscreen_event*>(data);

    bool set = event->fullscreen;

    server->output_manager->get_view_workspace(*view).set_fullscreen_view(*(server->output_manager), set ? OptionalRef<View>(view) : NullRef<View>);
}

void XDGPopup::destroy_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* popup = get_listener_data<XDGPopup*>(listener);

    server->listeners.clear_listeners(popup);
    delete popup;
}

void XDGPopup::new_popup_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* popup = get_listener_data<XDGPopup*>(listener);
    auto* wlr_popup = static_cast<struct wlr_xdg_popup*>(data);

    create_xdg_popup(*server, wlr_popup, popup->parent);
}

void XDGPopup::map_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* popup = get_listener_data<XDGPopup*>(listener);

    popup->parent->get_views_output(*server).and_then([popup](const auto& output) {
        wlr_surface_send_enter(popup->wlr_popup->base->surface, output.wlr_output);
    });
}
