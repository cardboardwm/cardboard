#ifdef __cplusplus
extern "C" {
#endif
/*
 * This an unstable interface of wlroots. No guarantees are made regarding the
 * future consistency of this API.
 */
#ifndef WLR_USE_UNSTABLE
#error "Add -DWLR_USE_UNSTABLE to enable unstable wlroots features"
#endif

#ifndef WLR_RENDER_INTERFACE_H
#define WLR_RENDER_INTERFACE_H

#include <wlr_cpp/config.h>

#if !WLR_HAS_X11_BACKEND && !WLR_HAS_XWAYLAND
#ifndef MESA_EGL_NO_X11_HEADERS
#define MESA_EGL_NO_X11_HEADERS
#endif
#ifndef EGL_NO_X11
#define EGL_NO_X11
#endif
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdbool.h>
#include <wayland-server-protocol.h>
#include <wlr_cpp/render/wlr_renderer.h>
#include <wlr_cpp/render/wlr_texture.h>
#include <wlr_cpp/types/wlr_box.h>
#include <wlr_cpp/types/wlr_output.h>
#include <wlr_cpp/render/dmabuf.h>

struct wlr_renderer_impl {
	void (*begin)(struct wlr_renderer *renderer, uint32_t width,
		uint32_t height);
	void (*end)(struct wlr_renderer *renderer);
	void (*clear)(struct wlr_renderer *renderer, const float *color);
	void (*scissor)(struct wlr_renderer *renderer, struct wlr_box *box);
	bool (*render_texture_with_matrix)(struct wlr_renderer *renderer,
		struct wlr_texture *texture, const float *matrix,
		float alpha);
	void (*render_quad_with_matrix)(struct wlr_renderer *renderer,
		const float *color, const float *matrix);
	void (*render_ellipse_with_matrix)(struct wlr_renderer *renderer,
		const float *color, const float *matrix);
	const enum wl_shm_format *(*formats)(
		struct wlr_renderer *renderer, size_t *len);
	bool (*format_supported)(struct wlr_renderer *renderer,
		enum wl_shm_format fmt);
	bool (*resource_is_wl_drm_buffer)(struct wlr_renderer *renderer,
		struct wl_resource *resource);
	void (*wl_drm_buffer_get_size)(struct wlr_renderer *renderer,
		struct wl_resource *buffer, int *width, int *height);
	const struct wlr_drm_format_set *(*get_dmabuf_formats)(
		struct wlr_renderer *renderer);
	enum wl_shm_format (*preferred_read_format)(struct wlr_renderer *renderer);
	bool (*read_pixels)(struct wlr_renderer *renderer, enum wl_shm_format fmt,
		uint32_t *flags, uint32_t stride, uint32_t width, uint32_t height,
		uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
		void *data);
	struct wlr_texture *(*texture_from_pixels)(struct wlr_renderer *renderer,
		enum wl_shm_format fmt, uint32_t stride, uint32_t width,
		uint32_t height, const void *data);
	struct wlr_texture *(*texture_from_wl_drm)(struct wlr_renderer *renderer,
		struct wl_resource *data);
	struct wlr_texture *(*texture_from_dmabuf)(struct wlr_renderer *renderer,
		struct wlr_dmabuf_attributes *attribs);
	void (*destroy)(struct wlr_renderer *renderer);
	void (*init_wl_display)(struct wlr_renderer *renderer,
		struct wl_display *wl_display);
};

void wlr_renderer_init(struct wlr_renderer *renderer,
	const struct wlr_renderer_impl *impl);

struct wlr_texture_impl {
	void (*get_size)(struct wlr_texture *texture, int *width, int *height);
	bool (*is_opaque)(struct wlr_texture *texture);
	bool (*write_pixels)(struct wlr_texture *texture,
		uint32_t stride, uint32_t width, uint32_t height,
		uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
		const void *data);
	bool (*to_dmabuf)(struct wlr_texture *texture,
		struct wlr_dmabuf_attributes *attribs);
	void (*destroy)(struct wlr_texture *texture);
};

void wlr_texture_init(struct wlr_texture *texture,
	const struct wlr_texture_impl *impl);

#endif
#ifdef __cplusplus
}
#endif
