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
    , mapped(false)
    , geometry { 0, 0, 0, 0 }
    , x(0)
    , y(0)
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

wlr_output* View::get_closest_output(wlr_output_layout* layout)
{
    wlr_output_layout_output* l_output;
    wlr_output* result = nullptr;
    double min_square_dist = std::numeric_limits<double>::max();
    double centre_x = x + geometry.x + (double)geometry.width / 2;
    double centre_y = y + geometry.y + (double)geometry.height / 2;

    // TODO: make clang-format put the brace on the same line
    wl_list_for_each(l_output, &layout->outputs, link)
    {
        auto* box = wlr_output_layout_get_box(layout, l_output->output);
        double o_centre_x = box->x + (double)box->width / 2;
        double o_centre_y = box->y + (double)box->height / 2;

        double square_dist = (o_centre_x - centre_x) * (o_centre_x - centre_x) + (o_centre_y - centre_y) * (o_centre_y - centre_y);
        if (square_dist < min_square_dist) {
            min_square_dist = square_dist;
            result = l_output->output;
        }
    }

    return result;
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
    server->focus_view(view);

    wlr_xdg_surface_get_geometry(view->xdg_surface, &view->geometry);
    server->tiles.add_view(view, prev_focused);
    server->tiles.fit_view_on_screen(view);
}
void xdg_surface_unmap_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    view->mapped = false;
    server->tiles.remove_view(view);

    auto it = std::find(server->focused_views.begin(), server->focused_views.end(), view);
    assert(it != server->focused_views.end());
    server->focused_views.erase(it);

    // focus last focused window
    if (!server->focused_views.empty()) {
        server->focus_view(server->focused_views.front());
        server->tiles.fit_view_on_screen(server->focused_views.front());
    }
}
void xdg_surface_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    View* view = get_listener_data<View*>(listener);
    Server* server = get_server(listener);

    server->listeners.clear_listeners(view);

    server->views.remove_if([view](auto& x) {
        return view == &x;
    });
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

        server->tiles.arrange_tiles();

        server->tiles.fit_view_on_screen(server->get_focused_view());
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
