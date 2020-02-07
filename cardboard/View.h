#ifndef __CARDBOARD_VIEW_H_
#define __CARDBOARD_VIEW_H_

#include <wayland-server.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>

#include <list>

struct Server;

struct View {
    struct wlr_xdg_surface* xdg_surface;
    bool mapped;
    int x, y;

    View(struct wlr_xdg_surface* xdg_surface);
    // Gets the child xdg surface of this view's toplevel xdg surface sitting under the cursor, if exists.
    // sx and sy and the given output layout relative coordinates (lx and ly), relative to that surface
    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy);
};

void xdg_surface_map_handler(struct wl_listener* listener, void* data);
void xdg_surface_unmap_handler(struct wl_listener* listener, void* data);
void xdg_surface_destroy_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_move_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data);

void create_view(Server* server, View&& view);

#endif // __CARDBOARD_VIEW_H_
