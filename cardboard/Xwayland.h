#ifndef CARDBOARD_XWAYLAND_H_INCLUDED
#define CARDBOARD_XWAYLAND_H_INCLUDED

extern "C" {
#include <wayland-server.h>
}

#include "View.h"

#include <wlr_cpp_fixes/xwayland.h>

class XwaylandView final : public View {
public:
    // This is pretty ugly, but remember that Xwayland by itself is ugly.
    // Views are supposed to not share state with the server, but X being X needs some info.
    Server* server;

    struct wlr_xwayland_surface* xwayland_surface;
    /// Stores listeners that are active only when the view is mapped. They are removed when unmapping.
    std::array<struct wl_listener*, 2> map_unmap_listeners;

    XwaylandView(Server* server, struct wlr_xwayland_surface* xwayland_surface);
    ~XwaylandView() = default;

    struct wlr_surface* get_surface() final;
    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy) final;
    void resize(int width, int height) final;
    void move(int x, int y) final;
    void prepare(Server& server) final;
    void set_activated(bool activated) final;
    void set_fullscreen(bool fullscreen) final;
    void for_each_surface(wlr_surface_iterator_func_t iterator, void* data) final;
    bool is_transient_for(View& ancestor) final;
    void close_popups() final;
    void close() final;

    void destroy();
    void unmap();

public:
    static void surface_map_handler(struct wl_listener* listener, void* data);
    static void surface_unmap_handler(struct wl_listener* listener, void* data);
    static void surface_destroy_handler(struct wl_listener* listener, void* data);
    static void surface_request_configure_handler(struct wl_listener* listener, void* data);

private:
    static void surface_commit_handler(struct wl_listener* listener, void* data);
    static void surface_request_fullscreen_handler(struct wl_listener* listener, void* data);
};

/// An "unmanaged" Xwayland surface. The "OR" stands for Override Redirect. Stuff like menus and tooltips.
struct XwaylandORSurface {
    Server* server;
    struct wlr_xwayland_surface* xwayland_surface;
    struct wl_listener* commit_listener;
    int lx, ly;
    bool mapped;

    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy);
    void map(Server* server);

public:
    static void surface_map_handler(struct wl_listener* listener, void* data);
    static void surface_unmap_handler(struct wl_listener* listener, void* data);
    static void surface_destroy_handler(struct wl_listener* listener, void* data);
    static void surface_request_configure_handler(struct wl_listener* listener, void* data);

private:
    static void surface_commit_handler(struct wl_listener* listener, void* data);
};

/// Registers an XwaylandORSurface.
XwaylandORSurface* create_xwayland_or_surface(Server* server, struct wlr_xwayland_surface* xwayland_surface);

#endif // CARDBOARD_XWAYLAND_H_INCLUDED
