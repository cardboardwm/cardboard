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

#ifndef WLR_RENDER_WLR_RENDERER_H
#define WLR_RENDER_WLR_RENDERER_H

#include <stdint.h>
#include <wayland-server-protocol.h>
#include <wlr_cpp/render/egl.h>
#include <wlr_cpp/render/wlr_texture.h>
#include <wlr_cpp/types/wlr_box.h>

enum wlr_renderer_read_pixels_flags {
	WLR_RENDERER_READ_PIXELS_Y_INVERT = 1,
};

struct wlr_renderer_impl;
struct wlr_drm_format_set;

struct wlr_renderer {
	const struct wlr_renderer_impl *impl;

	struct {
		struct wl_signal destroy;
	} events;
};

struct wlr_renderer *wlr_renderer_autocreate(struct wlr_egl *egl, EGLenum platform,
	void *remote_display, EGLint *config_attribs, EGLint visual_id);

void wlr_renderer_begin(struct wlr_renderer *r, int width, int height);
void wlr_renderer_end(struct wlr_renderer *r);
void wlr_renderer_clear(struct wlr_renderer *r, const float *color);
/**
 * Defines a scissor box. Only pixels that lie within the scissor box can be
 * modified by drawing functions. Providing a NULL `box` disables the scissor
 * box.
 */
void wlr_renderer_scissor(struct wlr_renderer *r, struct wlr_box *box);
/**
 * Renders the requested texture.
 */
bool wlr_render_texture(struct wlr_renderer *r, struct wlr_texture *texture,
	const float *projection, int x, int y, float alpha);
/**
 * Renders the requested texture using the provided matrix.
 */
bool wlr_render_texture_with_matrix(struct wlr_renderer *r,
	struct wlr_texture *texture, const float *matrix, float alpha);
/**
 * Renders a solid rectangle in the specified color.
 */
void wlr_render_rect(struct wlr_renderer *r, const struct wlr_box *box,
	const float *color, const float *projection);
/**
 * Renders a solid quadrangle in the specified color with the specified matrix.
 */
void wlr_render_quad_with_matrix(struct wlr_renderer *r,
	const float *color, const float *matrix);
/**
 * Renders a solid ellipse in the specified color.
 */
void wlr_render_ellipse(struct wlr_renderer *r, const struct wlr_box *box,
	const float *color, const float *projection);
/**
 * Renders a solid ellipse in the specified color with the specified matrix.
 */
void wlr_render_ellipse_with_matrix(struct wlr_renderer *r,
	const float *color, const float *matrix);
/**
 * Returns a list of pixel formats supported by this renderer.
 */
const enum wl_shm_format *wlr_renderer_get_formats(struct wlr_renderer *r,
	size_t *len);
/**
 * Returns true if this wl_buffer is a wl_drm buffer.
 */
bool wlr_renderer_resource_is_wl_drm_buffer(struct wlr_renderer *renderer,
	struct wl_resource *buffer);
/**
 * Gets the width and height of a wl_drm buffer.
 */
void wlr_renderer_wl_drm_buffer_get_size(struct wlr_renderer *renderer,
	struct wl_resource *buffer, int *width, int *height);
/**
 * Get the available DMA-BUF formats.
 */
const struct wlr_drm_format_set *wlr_renderer_get_dmabuf_formats(
	struct wlr_renderer *renderer);
/**
 * Reads out of pixels of the currently bound surface into data. `stride` is in
 * bytes.
 *
 * If `flags` is not NULl, the caller indicates that it accepts frame flags
 * defined in `enum wlr_renderer_read_pixels_flags`.
 */
bool wlr_renderer_read_pixels(struct wlr_renderer *r, enum wl_shm_format fmt,
	uint32_t *flags, uint32_t stride, uint32_t width, uint32_t height,
	uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, void *data);
/**
 * Checks if a format is supported.
 */
bool wlr_renderer_format_supported(struct wlr_renderer *r,
	enum wl_shm_format fmt);
void wlr_renderer_init_wl_display(struct wlr_renderer *r,
	struct wl_display *wl_display);
/**
 * Destroys this wlr_renderer. Textures must be destroyed separately.
 */
void wlr_renderer_destroy(struct wlr_renderer *renderer);

#endif
#ifdef __cplusplus
}
#endif
