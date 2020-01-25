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

#ifndef WLR_INTERFACES_WLR_OUTPUT_H
#define WLR_INTERFACES_WLR_OUTPUT_H

#include <stdbool.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_box.h>
#include <wlr_cpp/types/wlr_output.h>

struct wlr_output_impl {
	bool (*set_cursor)(struct wlr_output *output, struct wlr_texture *texture,
		int32_t scale, enum wl_output_transform transform,
		int32_t hotspot_x, int32_t hotspot_y, bool update_texture);
	bool (*move_cursor)(struct wlr_output *output, int x, int y);
	void (*destroy)(struct wlr_output *output);
	bool (*attach_render)(struct wlr_output *output, int *buffer_age);
	bool (*commit)(struct wlr_output *output);
	bool (*set_gamma)(struct wlr_output *output, size_t size,
		const uint16_t *r, const uint16_t *g, const uint16_t *b);
	size_t (*get_gamma_size)(struct wlr_output *output);
	bool (*export_dmabuf)(struct wlr_output *output,
		struct wlr_dmabuf_attributes *attribs);
	bool (*schedule_frame)(struct wlr_output *output);
	bool (*attach_buffer)(struct wlr_output *output, struct wlr_buffer *buffer);
};

void wlr_output_init(struct wlr_output *output, struct wlr_backend *backend,
	const struct wlr_output_impl *impl, struct wl_display *display);
void wlr_output_update_mode(struct wlr_output *output,
	struct wlr_output_mode *mode);
void wlr_output_update_custom_mode(struct wlr_output *output, int32_t width,
	int32_t height, int32_t refresh);
void wlr_output_update_enabled(struct wlr_output *output, bool enabled);
void wlr_output_update_needs_frame(struct wlr_output *output);
void wlr_output_damage_whole(struct wlr_output *output);
void wlr_output_send_frame(struct wlr_output *output);
void wlr_output_send_present(struct wlr_output *output,
	struct wlr_output_event_present *event);

#endif
#ifdef __cplusplus
}
#endif
