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

#ifndef WLR_BACKEND_DRM_H
#define WLR_BACKEND_DRM_H

#include <wayland-server-core.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/backend/session.h>
#include <wlr_cpp/types/wlr_output.h>

/**
 * Creates a DRM backend using the specified GPU file descriptor (typically from
 * a device node in /dev/dri).
 *
 * To slave this to another DRM backend, pass it as the parent (which _must_ be
 * a DRM backend, other kinds of backends raise SIGABRT).
 */
struct wlr_backend *wlr_drm_backend_create(struct wl_display *display,
	struct wlr_session *session, int gpu_fd, struct wlr_backend *parent,
	wlr_renderer_create_func_t create_renderer_func);

bool wlr_backend_is_drm(struct wlr_backend *backend);
bool wlr_output_is_drm(struct wlr_output *output);

/**
 * Add mode to the list of available modes
 */
typedef struct _drmModeModeInfo drmModeModeInfo;
bool wlr_drm_connector_add_mode(struct wlr_output *output, const drmModeModeInfo *mode);

#endif
#ifdef __cplusplus
}
#endif
