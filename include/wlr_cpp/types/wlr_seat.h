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

#ifndef WLR_TYPES_WLR_SEAT_H
#define WLR_TYPES_WLR_SEAT_H

#include <time.h>
#include <wayland-server-core.h>
#include <wlr_cpp/types/wlr_input_device.h>
#include <wlr_cpp/types/wlr_keyboard.h>
#include <wlr_cpp/types/wlr_surface.h>

#define WLR_SERIAL_RINGSET_SIZE 128

struct wlr_serial_range {
	uint32_t min_incl;
	uint32_t max_incl;
};
struct wlr_serial_ringset {
	struct wlr_serial_range data[WLR_SERIAL_RINGSET_SIZE];
	int end;
	int count;
};

/**
 * Contains state for a single client's bound wl_seat resource and can be used
 * to issue input events to that client. The lifetime of these objects is
 * managed by wlr_seat; some may be NULL.
 */
struct wlr_seat_client {
	struct wl_client *client;
	struct wlr_seat *seat;
	struct wl_list link;

	// lists of wl_resource
	struct wl_list resources;
	struct wl_list pointers;
	struct wl_list keyboards;
	struct wl_list touches;
	struct wl_list data_devices;

	struct {
		struct wl_signal destroy;
	} events;

	// set of serials which were sent to the client on this seat
	// for use by wlr_seat_client_{next_serial,validate_event_serial}
	struct wlr_serial_ringset serials;
};

struct wlr_touch_point {
	int32_t touch_id;
	struct wlr_surface *surface;
	struct wlr_seat_client *client;

	struct wlr_surface *focus_surface;
	struct wlr_seat_client *focus_client;
	double sx, sy;

	struct wl_listener surface_destroy;
	struct wl_listener focus_surface_destroy;
	struct wl_listener client_destroy;

	struct {
		struct wl_signal destroy;
	} events;

	struct wl_list link;
};

struct wlr_seat_pointer_grab;

struct wlr_pointer_grab_interface {
	void (*enter)(struct wlr_seat_pointer_grab *grab,
			struct wlr_surface *surface, double sx, double sy);
	void (*motion)(struct wlr_seat_pointer_grab *grab, uint32_t time_msec,
			double sx, double sy);
	uint32_t (*button)(struct wlr_seat_pointer_grab *grab, uint32_t time_msec,
			uint32_t button, enum wlr_button_state state);
	void (*axis)(struct wlr_seat_pointer_grab *grab, uint32_t time_msec,
			enum wlr_axis_orientation orientation, double value,
			int32_t value_discrete, enum wlr_axis_source source);
	void (*frame)(struct wlr_seat_pointer_grab *grab);
	void (*cancel)(struct wlr_seat_pointer_grab *grab);
};

struct wlr_seat_keyboard_grab;

struct wlr_keyboard_grab_interface {
	void (*enter)(struct wlr_seat_keyboard_grab *grab,
			struct wlr_surface *surface, uint32_t keycodes[],
			size_t num_keycodes, struct wlr_keyboard_modifiers *modifiers);
	void (*key)(struct wlr_seat_keyboard_grab *grab, uint32_t time_msec,
			uint32_t key, uint32_t state);
	void (*modifiers)(struct wlr_seat_keyboard_grab *grab,
			struct wlr_keyboard_modifiers *modifiers);
	void (*cancel)(struct wlr_seat_keyboard_grab *grab);
};

struct wlr_seat_touch_grab;

struct wlr_touch_grab_interface {
	uint32_t (*down)(struct wlr_seat_touch_grab *grab, uint32_t time_msec,
			struct wlr_touch_point *point);
	void (*up)(struct wlr_seat_touch_grab *grab, uint32_t time_msec,
			struct wlr_touch_point *point);
	void (*motion)(struct wlr_seat_touch_grab *grab, uint32_t time_msec,
			struct wlr_touch_point *point);
	void (*enter)(struct wlr_seat_touch_grab *grab, uint32_t time_msec,
			struct wlr_touch_point *point);
	// XXX this will conflict with the actual touch cancel which is different so
	// we need to rename this
	void (*cancel)(struct wlr_seat_touch_grab *grab);
};

/**
 * Passed to `wlr_seat_touch_start_grab()` to start a grab of the touch device.
 * The grabber is responsible for handling touch events for the seat.
 */
struct wlr_seat_touch_grab {
	const struct wlr_touch_grab_interface *interface;
	struct wlr_seat *seat;
	void *data;
};

/**
 * Passed to `wlr_seat_keyboard_start_grab()` to start a grab of the keyboard.
 * The grabber is responsible for handling keyboard events for the seat.
 */
struct wlr_seat_keyboard_grab {
	const struct wlr_keyboard_grab_interface *interface;
	struct wlr_seat *seat;
	void *data;
};

/**
 * Passed to `wlr_seat_pointer_start_grab()` to start a grab of the pointer. The
 * grabber is responsible for handling pointer events for the seat.
 */
struct wlr_seat_pointer_grab {
	const struct wlr_pointer_grab_interface *interface;
	struct wlr_seat *seat;
	void *data;
};

#define WLR_POINTER_BUTTONS_CAP 16

struct wlr_seat_pointer_state {
	struct wlr_seat *seat;
	struct wlr_seat_client *focused_client;
	struct wlr_surface *focused_surface;
	double sx, sy;

	struct wlr_seat_pointer_grab *grab;
	struct wlr_seat_pointer_grab *default_grab;

	uint32_t buttons[WLR_POINTER_BUTTONS_CAP];
	size_t button_count;
	uint32_t grab_button;
	uint32_t grab_serial;
	uint32_t grab_time;

	struct wl_listener surface_destroy;

	struct {
		struct wl_signal focus_change; // wlr_seat_pointer_focus_change_event
	} events;
};

// TODO: May be useful to be able to simulate keyboard input events
struct wlr_seat_keyboard_state {
	struct wlr_seat *seat;
	struct wlr_keyboard *keyboard;

	struct wlr_seat_client *focused_client;
	struct wlr_surface *focused_surface;

	struct wl_listener keyboard_destroy;
	struct wl_listener keyboard_keymap;
	struct wl_listener keyboard_repeat_info;

	struct wl_listener surface_destroy;

	struct wlr_seat_keyboard_grab *grab;
	struct wlr_seat_keyboard_grab *default_grab;

	struct {
		struct wl_signal focus_change; // wlr_seat_keyboard_focus_change_event
	} events;
};

struct wlr_seat_touch_state {
	struct wlr_seat *seat;
	struct wl_list touch_points; // wlr_touch_point::link

	uint32_t grab_serial;
	uint32_t grab_id;

	struct wlr_seat_touch_grab *grab;
	struct wlr_seat_touch_grab *default_grab;
};

struct wlr_primary_selection_source;

struct wlr_seat {
	struct wl_global *global;
	struct wl_display *display;
	struct wl_list clients;

	char *name;
	uint32_t capabilities;
	struct timespec last_event;

	struct wlr_data_source *selection_source;
	uint32_t selection_serial;
	struct wl_list selection_offers; // wlr_data_offer::link

	struct wlr_primary_selection_source *primary_selection_source;
	uint32_t primary_selection_serial;

	// `drag` goes away before `drag_source`, when the implicit grab ends
	struct wlr_drag *drag;
	struct wlr_data_source *drag_source;
	uint32_t drag_serial;
	struct wl_list drag_offers; // wlr_data_offer::link

	struct wlr_seat_pointer_state pointer_state;
	struct wlr_seat_keyboard_state keyboard_state;
	struct wlr_seat_touch_state touch_state;

	struct wl_listener display_destroy;
	struct wl_listener selection_source_destroy;
	struct wl_listener primary_selection_source_destroy;
	struct wl_listener drag_source_destroy;

	struct {
		struct wl_signal pointer_grab_begin;
		struct wl_signal pointer_grab_end;

		struct wl_signal keyboard_grab_begin;
		struct wl_signal keyboard_grab_end;

		struct wl_signal touch_grab_begin;
		struct wl_signal touch_grab_end;

		// wlr_seat_pointer_request_set_cursor_event
		struct wl_signal request_set_cursor;

		// wlr_seat_request_set_selection_event
		struct wl_signal request_set_selection;
		struct wl_signal set_selection;
		// wlr_seat_request_set_primary_selection_event
		struct wl_signal request_set_primary_selection;
		struct wl_signal set_primary_selection;

		// wlr_seat_request_start_drag_event
		struct wl_signal request_start_drag;
		struct wl_signal start_drag; // wlr_drag

		struct wl_signal destroy;
	} events;

	void *data;
};

struct wlr_seat_pointer_request_set_cursor_event {
	struct wlr_seat_client *seat_client;
	struct wlr_surface *surface;
	uint32_t serial;
	int32_t hotspot_x, hotspot_y;
};

struct wlr_seat_request_set_selection_event {
	struct wlr_data_source *source;
	uint32_t serial;
};

struct wlr_seat_request_set_primary_selection_event {
	struct wlr_primary_selection_source *source;
	uint32_t serial;
};

struct wlr_seat_request_start_drag_event {
	struct wlr_drag *drag;
	struct wlr_surface *origin;
	uint32_t serial;
};

struct wlr_seat_pointer_focus_change_event {
	struct wlr_seat *seat;
	struct wlr_surface *old_surface, *new_surface;
	double sx, sy;
};

struct wlr_seat_keyboard_focus_change_event {
	struct wlr_seat *seat;
	struct wlr_surface *old_surface, *new_surface;
};

/**
 * Allocates a new wlr_seat and adds a wl_seat global to the display.
 */
struct wlr_seat *wlr_seat_create(struct wl_display *display, const char *name);
/**
 * Destroys a wlr_seat and removes its wl_seat global.
 */
void wlr_seat_destroy(struct wlr_seat *wlr_seat);
/**
 * Gets a wlr_seat_client for the specified client, or returns NULL if no
 * client is bound for that client.
 */
struct wlr_seat_client *wlr_seat_client_for_wl_client(struct wlr_seat *wlr_seat,
		struct wl_client *wl_client);
/**
 * Updates the capabilities available on this seat.
 * Will automatically send them to all clients.
 */
void wlr_seat_set_capabilities(struct wlr_seat *wlr_seat,
		uint32_t capabilities);
/**
 * Updates the name of this seat.
 * Will automatically send it to all clients.
 */
void wlr_seat_set_name(struct wlr_seat *wlr_seat, const char *name);

/**
 * Whether or not the surface has pointer focus
 */
bool wlr_seat_pointer_surface_has_focus(struct wlr_seat *wlr_seat,
		struct wlr_surface *surface);

/**
 * Send a pointer enter event to the given surface and consider it to be the
 * focused surface for the pointer. This will send a leave event to the last
 * surface that was entered. Coordinates for the enter event are surface-local.
 * Compositor should use `wlr_seat_pointer_notify_enter()` to change pointer
 * focus to respect pointer grabs.
 */
void wlr_seat_pointer_enter(struct wlr_seat *wlr_seat,
		struct wlr_surface *surface, double sx, double sy);

/**
 * Clear the focused surface for the pointer and leave all entered surfaces.
 */
void wlr_seat_pointer_clear_focus(struct wlr_seat *wlr_seat);

/**
 * Send a motion event to the surface with pointer focus. Coordinates for the
 * motion event are surface-local. Compositors should use
 * `wlr_seat_pointer_notify_motion()` to send motion events to respect pointer
 * grabs.
 */
void wlr_seat_pointer_send_motion(struct wlr_seat *wlr_seat, uint32_t time_msec,
		double sx, double sy);

/**
 * Send a button event to the surface with pointer focus. Coordinates for the
 * button event are surface-local. Returns the serial. Compositors should use
 * `wlr_seat_pointer_notify_button()` to send button events to respect pointer
 * grabs.
 */
uint32_t wlr_seat_pointer_send_button(struct wlr_seat *wlr_seat,
		uint32_t time_msec, uint32_t button, enum wlr_button_state state);

/**
 * Send an axis event to the surface with pointer focus. Compositors should use
 * `wlr_seat_pointer_notify_axis()` to send axis events to respect pointer
 * grabs.
 **/
void wlr_seat_pointer_send_axis(struct wlr_seat *wlr_seat, uint32_t time_msec,
		enum wlr_axis_orientation orientation, double value,
		int32_t value_discrete, enum wlr_axis_source source);

/**
 * Send a frame event to the surface with pointer focus. Compositors should use
 * `wlr_seat_pointer_notify_frame()` to send axis events to respect pointer
 * grabs.
 */
void wlr_seat_pointer_send_frame(struct wlr_seat *wlr_seat);

/**
 * Start a grab of the pointer of this seat. The grabber is responsible for
 * handling all pointer events until the grab ends.
 */
void wlr_seat_pointer_start_grab(struct wlr_seat *wlr_seat,
		struct wlr_seat_pointer_grab *grab);

/**
 * End the grab of the pointer of this seat. This reverts the grab back to the
 * default grab for the pointer.
 */
void wlr_seat_pointer_end_grab(struct wlr_seat *wlr_seat);

/**
 * Notify the seat of a pointer enter event to the given surface and request it
 * to be the focused surface for the pointer. Pass surface-local coordinates
 * where the enter occurred.
 */
void wlr_seat_pointer_notify_enter(struct wlr_seat *wlr_seat,
		struct wlr_surface *surface, double sx, double sy);

/**
 * Notify the seat of motion over the given surface. Pass surface-local
 * coordinates where the pointer motion occurred.
 */
void wlr_seat_pointer_notify_motion(struct wlr_seat *wlr_seat,
		uint32_t time_msec, double sx, double sy);

/**
 * Notify the seat that a button has been pressed. Returns the serial of the
 * button press or zero if no button press was sent.
 */
uint32_t wlr_seat_pointer_notify_button(struct wlr_seat *wlr_seat,
		uint32_t time_msec, uint32_t button, enum wlr_button_state state);

/**
 * Notify the seat of an axis event.
 */
void wlr_seat_pointer_notify_axis(struct wlr_seat *wlr_seat, uint32_t time_msec,
		enum wlr_axis_orientation orientation, double value,
		int32_t value_discrete, enum wlr_axis_source source);

/**
 * Notify the seat of a frame event. Frame events are sent to end a group of
 * events that logically belong together. Motion, button and axis events should
 * all be followed by a frame event.
 */
void wlr_seat_pointer_notify_frame(struct wlr_seat *wlr_seat);

/**
 * Whether or not the pointer has a grab other than the default grab.
 */
bool wlr_seat_pointer_has_grab(struct wlr_seat *seat);

/**
 * Set this keyboard as the active keyboard for the seat.
 */
void wlr_seat_set_keyboard(struct wlr_seat *seat, struct wlr_input_device *dev);

/**
 * Get the active keyboard for the seat.
 */
struct wlr_keyboard *wlr_seat_get_keyboard(struct wlr_seat *seat);

/**
 * Start a grab of the keyboard of this seat. The grabber is responsible for
 * handling all keyboard events until the grab ends.
 */
void wlr_seat_keyboard_start_grab(struct wlr_seat *wlr_seat,
		struct wlr_seat_keyboard_grab *grab);

/**
 * End the grab of the keyboard of this seat. This reverts the grab back to the
 * default grab for the keyboard.
 */
void wlr_seat_keyboard_end_grab(struct wlr_seat *wlr_seat);

/**
 * Send the keyboard key to focused keyboard resources. Compositors should use
 * `wlr_seat_notify_key()` to respect keyboard grabs.
 */
void wlr_seat_keyboard_send_key(struct wlr_seat *seat, uint32_t time_msec,
		uint32_t key, uint32_t state);

/**
 * Notify the seat that a key has been pressed on the keyboard. Defers to any
 * keyboard grabs.
 */
void wlr_seat_keyboard_notify_key(struct wlr_seat *seat, uint32_t time_msec,
		uint32_t key, uint32_t state);

/**
 * Send the modifier state to focused keyboard resources. Compositors should use
 * `wlr_seat_keyboard_notify_modifiers()` to respect any keyboard grabs.
 */
void wlr_seat_keyboard_send_modifiers(struct wlr_seat *seat,
		struct wlr_keyboard_modifiers *modifiers);

/**
 * Notify the seat that the modifiers for the keyboard have changed. Defers to
 * any keyboard grabs.
 */
void wlr_seat_keyboard_notify_modifiers(struct wlr_seat *seat,
		struct wlr_keyboard_modifiers *modifiers);

/**
 * Notify the seat that the keyboard focus has changed and request it to be the
 * focused surface for this keyboard. Defers to any current grab of the seat's
 * keyboard.
 */
void wlr_seat_keyboard_notify_enter(struct wlr_seat *seat,
		struct wlr_surface *surface, uint32_t keycodes[], size_t num_keycodes,
		struct wlr_keyboard_modifiers *modifiers);

/**
 * Send a keyboard enter event to the given surface and consider it to be the
 * focused surface for the keyboard. This will send a leave event to the last
 * surface that was entered. Compositors should use
 * `wlr_seat_keyboard_notify_enter()` to change keyboard focus to respect
 * keyboard grabs.
 */
void wlr_seat_keyboard_enter(struct wlr_seat *seat,
		struct wlr_surface *surface, uint32_t keycodes[], size_t num_keycodes,
		struct wlr_keyboard_modifiers *modifiers);

/**
 * Clear the focused surface for the keyboard and leave all entered surfaces.
 */
void wlr_seat_keyboard_clear_focus(struct wlr_seat *wlr_seat);

/**
 * Whether or not the keyboard has a grab other than the default grab
 */
bool wlr_seat_keyboard_has_grab(struct wlr_seat *seat);

/**
 * Start a grab of the touch device of this seat. The grabber is responsible for
 * handling all touch events until the grab ends.
 */
void wlr_seat_touch_start_grab(struct wlr_seat *wlr_seat,
		struct wlr_seat_touch_grab *grab);

/**
 * End the grab of the touch device of this seat. This reverts the grab back to
 * the default grab for the touch device.
 */
void wlr_seat_touch_end_grab(struct wlr_seat *wlr_seat);

/**
 * Get the active touch point with the given `touch_id`. If the touch point does
 * not exist or is no longer active, returns NULL.
 */
struct wlr_touch_point *wlr_seat_touch_get_point(struct wlr_seat *seat,
		int32_t touch_id);

/**
 * Notify the seat of a touch down on the given surface. Defers to any grab of
 * the touch device.
 */
uint32_t wlr_seat_touch_notify_down(struct wlr_seat *seat,
		struct wlr_surface *surface, uint32_t time_msec,
		int32_t touch_id, double sx, double sy);

/**
 * Notify the seat that the touch point given by `touch_id` is up. Defers to any
 * grab of the touch device.
 */
void wlr_seat_touch_notify_up(struct wlr_seat *seat, uint32_t time_msec,
		int32_t touch_id);

/**
 * Notify the seat that the touch point given by `touch_id` has moved. Defers to
 * any grab of the touch device. The seat should be notified of touch motion
 * even if the surface is not the owner of the touch point for processing by
 * grabs.
 */
void wlr_seat_touch_notify_motion(struct wlr_seat *seat, uint32_t time_msec,
		int32_t touch_id, double sx, double sy);

/**
 * Notify the seat that the touch point given by `touch_id` has entered a new
 * surface. The surface is required. To clear focus, use
 * `wlr_seat_touch_point_clear_focus()`.
 */
void wlr_seat_touch_point_focus(struct wlr_seat *seat,
		struct wlr_surface *surface, uint32_t time_msec,
		int32_t touch_id, double sx, double sy);

/**
 * Clear the focused surface for the touch point given by `touch_id`.
 */
void wlr_seat_touch_point_clear_focus(struct wlr_seat *seat, uint32_t time_msec,
		int32_t touch_id);

/**
 * Send a touch down event to the client of the given surface. All future touch
 * events for this point will go to this surface. If the touch down is valid,
 * this will add a new touch point with the given `touch_id`. The touch down may
 * not be valid if the surface seat client does not accept touch input.
 * Coordinates are surface-local. Compositors should use
 * `wlr_seat_touch_notify_down()` to respect any grabs of the touch device.
 */
uint32_t wlr_seat_touch_send_down(struct wlr_seat *seat,
		struct wlr_surface *surface, uint32_t time_msec,
		int32_t touch_id, double sx, double sy);

/**
 * Send a touch up event for the touch point given by the `touch_id`. The event
 * will go to the client for the surface given in the cooresponding touch down
 * event. This will remove the touch point. Compositors should use
 * `wlr_seat_touch_notify_up()` to respect any grabs of the touch device.
 */
void wlr_seat_touch_send_up(struct wlr_seat *seat, uint32_t time_msec,
		int32_t touch_id);

/**
 * Send a touch motion event for the touch point given by the `touch_id`. The
 * event will go to the client for the surface given in the corresponding touch
 * down event. Compositors should use `wlr_seat_touch_notify_motion()` to
 * respect any grabs of the touch device.
 */
void wlr_seat_touch_send_motion(struct wlr_seat *seat, uint32_t time_msec,
		int32_t touch_id, double sx, double sy);

/**
 * How many touch points are currently down for the seat.
 */
int wlr_seat_touch_num_points(struct wlr_seat *seat);

/**
 * Whether or not the seat has a touch grab other than the default grab.
 */
bool wlr_seat_touch_has_grab(struct wlr_seat *seat);

/**
 * Check whether this serial is valid to start a grab action such as an
 * interactive move or resize.
 */
bool wlr_seat_validate_grab_serial(struct wlr_seat *seat, uint32_t serial);

/**
 * Check whether this serial is valid to start a pointer grab action.
 */
bool wlr_seat_validate_pointer_grab_serial(struct wlr_seat *seat,
	struct wlr_surface *origin, uint32_t serial);

/**
 * Check whether this serial is valid to start a touch grab action. If it's the
 * case and point_ptr is non-NULL, *point_ptr is set to the touch point matching
 * the serial.
 */
bool wlr_seat_validate_touch_grab_serial(struct wlr_seat *seat,
	struct wlr_surface *origin, uint32_t serial,
	struct wlr_touch_point **point_ptr);

/**
 * Return a new serial (from wl_display_serial_next()) for the client, and
 * update the seat client's set of valid serials. Use this for all input
 * events; otherwise wlr_seat_client_validate_event_serial() may fail when
 * handed a correctly functioning client's request serials.
 */
uint32_t wlr_seat_client_next_serial(struct wlr_seat_client *client);

/**
 * Return true if the serial number could have been produced by
 * wlr_seat_client_next_serial() and is "older" (by less than UINT32_MAX/2) than
 * the current display serial value.
 *
 * This function should have no false negatives, and the only false positive
 * responses allowed are for elements that are still "older" than the current
 * display serial value and also older than all serial values remaining in
 * the seat client's serial ring buffer, if that buffer is also full.
 */
bool wlr_seat_client_validate_event_serial(struct wlr_seat_client *client,
	uint32_t serial);

/**
 * Get a seat client from a seat resource. Returns NULL if inert.
 */
struct wlr_seat_client *wlr_seat_client_from_resource(
	struct wl_resource *resource);

/**
 * Get a seat client from a pointer resource. Returns NULL if inert.
 */
struct wlr_seat_client *wlr_seat_client_from_pointer_resource(
	struct wl_resource *resource);

#endif
#ifdef __cplusplus
}
#endif
