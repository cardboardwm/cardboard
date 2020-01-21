#ifdef __cplusplus
extern "C" {
#endif
#ifndef WLR_TYPES_WLR_XDG_DECORATION_V1
#define WLR_TYPES_WLR_XDG_DECORATION_V1

#include <wayland-server-core.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>

enum wlr_xdg_toplevel_decoration_v1_mode {
	WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE = 0,
	WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE = 1,
	WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE = 2,
};

struct wlr_xdg_decoration_manager_v1 {
	struct wl_global *global;
	struct wl_list resources;
	struct wl_list decorations; // wlr_xdg_toplevel_decoration::link

	struct wl_listener display_destroy;

	struct {
		struct wl_signal new_toplevel_decoration; // struct wlr_xdg_toplevel_decoration *
		struct wl_signal destroy;
	} events;

	void *data;
};

struct wlr_xdg_toplevel_decoration_v1_configure {
	struct wl_list link; // wlr_xdg_toplevel_decoration::configure_list
	struct wlr_xdg_surface_configure *surface_configure;
	enum wlr_xdg_toplevel_decoration_v1_mode mode;
};

struct wlr_xdg_toplevel_decoration_v1 {
	struct wl_resource *resource;
	struct wlr_xdg_surface *surface;
	struct wlr_xdg_decoration_manager_v1 *manager;
	struct wl_list link; // wlr_xdg_decoration_manager_v1::link

	bool added;
	enum wlr_xdg_toplevel_decoration_v1_mode current_mode;
	enum wlr_xdg_toplevel_decoration_v1_mode client_pending_mode;
	enum wlr_xdg_toplevel_decoration_v1_mode server_pending_mode;

	struct wl_list configure_list; // wlr_xdg_toplevel_decoration_v1_configure::link

	struct {
		struct wl_signal destroy;
		struct wl_signal request_mode;
	} events;

	struct wl_listener surface_destroy;
	struct wl_listener surface_configure;
	struct wl_listener surface_ack_configure;
	struct wl_listener surface_commit;

	void *data;
};

struct wlr_xdg_decoration_manager_v1 *
	wlr_xdg_decoration_manager_v1_create(struct wl_display *display);
void wlr_xdg_decoration_manager_v1_destroy(
	struct wlr_xdg_decoration_manager_v1 *manager);

uint32_t wlr_xdg_toplevel_decoration_v1_set_mode(
	struct wlr_xdg_toplevel_decoration_v1 *decoration,
	enum wlr_xdg_toplevel_decoration_v1_mode mode);

#endif
#ifdef __cplusplus
}
#endif
