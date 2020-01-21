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

#ifndef WLR_TYPES_WLR_PRESENTATION_TIME_H
#define WLR_TYPES_WLR_PRESENTATION_TIME_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <wayland-server-core.h>

struct wlr_presentation {
	struct wl_global *global;
	struct wl_list resources; // wl_resource_get_link
	struct wl_list feedbacks; // wlr_presentation_feedback::link
	clockid_t clock;

	struct {
		struct wl_signal destroy;
	} events;

	struct wl_listener display_destroy;
};

struct wlr_presentation_feedback {
	struct wl_resource *resource;
	struct wlr_presentation *presentation;
	struct wlr_surface *surface;
	struct wl_list link; // wlr_presentation::feedbacks

	// The surface contents were committed.
	bool committed;
	// The surface contents were sampled by the compositor and are to be
	// presented on the next flip. Can become true only after committed becomes
	// true.
	bool sampled;

	struct wl_listener surface_commit;
	struct wl_listener surface_destroy;
};

struct wlr_presentation_event {
	struct wlr_output *output;
	uint64_t tv_sec;
	uint32_t tv_nsec;
	uint32_t refresh;
	uint64_t seq;
	uint32_t flags; // wp_presentation_feedback_kind
};

struct wlr_backend;

struct wlr_presentation *wlr_presentation_create(struct wl_display *display,
	struct wlr_backend *backend);
void wlr_presentation_destroy(struct wlr_presentation *presentation);
void wlr_presentation_send_surface_presented(
	struct wlr_presentation *presentation, struct wlr_surface *surface,
	struct wlr_presentation_event *event);
void wlr_presentation_surface_sampled(
	struct wlr_presentation *presentation, struct wlr_surface *surface);

#endif
#ifdef __cplusplus
}
#endif
