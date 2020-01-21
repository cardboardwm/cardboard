#ifndef __CARDBOARD_VIEW_H_
#define __CARDBOARD_VIEW_H_

#include <wayland-server.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>

#include <list>

struct Server;

struct View {
    Server* server;
    std::list<View>::iterator link;

    struct wlr_xdg_surface* xdg_surface;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    bool mapped;
    int x, y;

    View(Server* server, struct wlr_xdg_surface* xdg_surface);

    static void xdg_surface_map_handler(struct wl_listener* listener, void* data);
    static void xdg_surface_unmap_handler(struct wl_listener* listener, void* data);
    static void xdg_surface_destroy_handler(struct wl_listener* listener, void* data);
    static void xdg_toplevel_request_move_handler(struct wl_listener* listener, void* data);
    static void xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data);
};

#endif // __CARDBOARD_VIEW_H_
