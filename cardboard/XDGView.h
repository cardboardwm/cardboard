#ifndef __CARDBOARD_XDGVIEW_H_
#define __CARDBOARD_XDGVIEW_H_

#include <array>

#include "View.h"

class XDGView : public View {
public:
    struct wlr_xdg_surface* xdg_surface;
    std::array<struct wl_listener*, 3> map_unmap_listeners;

    XDGView(struct wlr_xdg_surface* xdg_surface);
    ~XDGView() = default;

    struct wlr_surface* get_surface();
    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy);
    void resize(int width, int height);
    void prepare(Server* server);
    void set_activated(bool activated);
    void for_each_surface(wlr_surface_iterator_func_t iterator, void* data);
};

void xdg_surface_map_handler(struct wl_listener* listener, void* data);
void xdg_surface_unmap_handler(struct wl_listener* listener, void* data);
void xdg_surface_destroy_handler(struct wl_listener* listener, void* data);
void xdg_surface_commit_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_move_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_XDGVIEW_H_
