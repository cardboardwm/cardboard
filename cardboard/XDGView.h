#ifndef CARDBOARD_XDGVIEW_H_INCLUDED
#define CARDBOARD_XDGVIEW_H_INCLUDED

#include <array>

#include "View.h"

class XDGView final : public View {
public:
    struct wlr_xdg_surface* xdg_surface;
    /// Stores listeners that are active only when the view is mapped. They are removed when unmapping.
    std::array<struct wl_listener*, 4> map_unmap_listeners;

    XDGView(struct wlr_xdg_surface* xdg_surface);
    ~XDGView() = default;

    struct wlr_surface* get_surface() final;
    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy) final;
    void resize(int width, int height) final;
    void prepare(Server& server) final;
    void set_activated(bool activated) final;
    void set_fullscreen(bool fullscreen) final;
    void for_each_surface(wlr_surface_iterator_func_t iterator, void* data) final;
    bool is_transient_for(View& ancestor) final;
    void close_popups() final;
    void close() final;

public:
    static void surface_map_handler(struct wl_listener* listener, void* data);
    static void surface_unmap_handler(struct wl_listener* listener, void* data);
    static void surface_destroy_handler(struct wl_listener* listener, void* data);
    static void surface_new_popup_handler(struct wl_listener* listener, void* data);

private:
    static void surface_commit_handler(struct wl_listener* listener, void* data);
    static void toplevel_request_move_handler(struct wl_listener* listener, void* data);
    static void toplevel_request_resize_handler(struct wl_listener* listener, void* data);
    static void toplevel_request_fullscreen_handler(struct wl_listener* listener, void* data);
};

struct XDGPopup {
    struct wlr_xdg_popup* wlr_popup;
    NotNullPointer<XDGView> parent;

    XDGPopup(struct wlr_xdg_popup*, NotNullPointer<XDGView>);

    void unconstrain(Server& server);

public:
    static void destroy_handler(struct wl_listener* listener, void* data);
    static void new_popup_handler(struct wl_listener* listener, void* data);
    static void map_handler(struct wl_listener* listener, void* data);
};

#endif // CARDBOARD_XDGVIEW_H_INCLUDED
