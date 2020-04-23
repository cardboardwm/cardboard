#ifndef __CARDBOARD_XWAYLAND_H_
#define __CARDBOARD_XWAYLAND_H_

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

    struct wlr_surface* get_surface() override;
    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy) override;
    void resize(int width, int height) override;
    void prepare(Server* server) override;
    void set_activated(bool activated) override;
    void set_fullscreen(bool fullscreen) override;
    void for_each_surface(wlr_surface_iterator_func_t iterator, void* data) override;
    bool is_transient_for(View* ancestor) override;
    void close_popups() override;
    void close() override;

    void destroy();
    void unmap();
};

void xwayland_surface_map_handler(struct wl_listener* listener, void* data);
void xwayland_surface_unmap_handler(struct wl_listener* listener, void* data);
void xwayland_surface_destroy_handler(struct wl_listener* listener, void* data);
void xwayland_surface_request_configure_handler(struct wl_listener* listener, void* data);
void xwayland_surface_commit_handler(struct wl_listener* listener, void* data);
void xwayland_surface_request_fullscreen_handler(struct wl_listener* listener, void* data);

/// An "unmanaged" Xwayland surface. The "OR" stands for Override Redirect. Stuff like menus and tooltips.
struct XwaylandORSurface {
    Server* server;
    struct wlr_xwayland_surface* xwayland_surface;
    struct wl_listener* commit_listener;
    int lx, ly;

    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy);
    void map(Server* server);
};

/// Registers an XwaylandORSurface.
XwaylandORSurface* create_xwayland_or_surface(Server* server, struct wlr_xwayland_surface* xwayland_surface);

void xwayland_or_surface_map_handler(struct wl_listener* listener, void* data);
void xwayland_or_surface_unmap_handler(struct wl_listener* listener, void* data);
void xwayland_or_surface_destroy_handler(struct wl_listener* listener, void* data);
void xwayland_or_surface_request_configure_handler(struct wl_listener* listener, void* data);
void xwayland_or_surface_commit_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_XWAYLAND_H_
