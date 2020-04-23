#ifndef __CARDBOARD_XDGVIEW_H_
#define __CARDBOARD_XDGVIEW_H_

#include <array>

#include "View.h"

class XDGView final : public View {
public:
    struct wlr_xdg_surface* xdg_surface;
    /// Stores listeners that are active only when the view is mapped. They are removed when unmapping.
    std::array<struct wl_listener*, 4> map_unmap_listeners;

    XDGView(struct wlr_xdg_surface* xdg_surface);
    ~XDGView() = default;

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
};

struct XDGPopup {
    struct wlr_xdg_popup* wlr_popup;
    XDGView* parent;

    void unconstrain(Server* server);
};

void create_xdg_popup(Server* server, struct wlr_xdg_popup* wlr_popup, XDGView* parent);

void xdg_surface_map_handler(struct wl_listener* listener, void* data);
void xdg_surface_unmap_handler(struct wl_listener* listener, void* data);
void xdg_surface_destroy_handler(struct wl_listener* listener, void* data);
void xdg_surface_commit_handler(struct wl_listener* listener, void* data);
void xdg_surface_new_popup_handler(struct wl_listener* listener, void* data);

void xdg_toplevel_request_move_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_fullscreen_handler(struct wl_listener* listener, void* data);

void xdg_popup_destroy_handler(struct wl_listener* listener, void* data);
void xdg_popup_new_popup_handler(struct wl_listener* listener, void* data);
void xdg_popup_map_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_XDGVIEW_H_
