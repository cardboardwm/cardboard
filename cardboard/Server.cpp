#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_compositor.h>
#include <wlr_cpp/types/wlr_cursor.h>
#include <wlr_cpp/types/wlr_data_device.h>
#include <wlr_cpp/types/wlr_output_layout.h>
#include <wlr_cpp/types/wlr_xcursor_manager.h>
#include <wlr_cpp/util/log.h>

#include "Server.h"

Server::Server()
{
    listeners.server = this;

    wl_display = wl_display_create();
    // let wlroots select the required hardware abstractions
    backend = wlr_backend_autocreate(wl_display, nullptr);

    renderer = wlr_backend_get_renderer(backend);
    wlr_renderer_init_wl_display(renderer, wl_display);

    wlr_compositor_create(wl_display, renderer);
    wlr_data_device_manager_create(wl_display); // for clipboard

    output_layout = wlr_output_layout_create();

    listeners.new_output.notify = EventListeners::new_output_handler;
    wl_signal_add(&backend->events.new_output, &listeners.new_output);

    // https://drewdevault.com/2018/07/29/Wayland-shells.html
    // TODO: implement layer-shell
    // TODO: implement Xwayland
    xdg_shell = wlr_xdg_shell_create(wl_display);
    listeners.new_xdg_surface.notify = EventListeners::new_xdg_surface_handler;
    wl_signal_add(&xdg_shell->events.new_surface, &listeners.new_xdg_surface);

    cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(cursor, output_layout);

    // the cursor manager loads xcursor images for all scale factors
    cursor_manager = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(cursor_manager, 1);

    // mouse cursor handlers
    listeners.cursor_motion.notify = EventListeners::cursor_motion_handler;
    listeners.cursor_motion_absolute.notify = EventListeners::cursor_motion_absolute_handler;
    listeners.cursor_button.notify = EventListeners::cursor_button_handler;
    listeners.cursor_axis.notify = EventListeners::cursor_axis_handler;
    listeners.cursor_frame.notify = EventListeners::cursor_frame_handler;
    wl_signal_add(&cursor->events.motion, &listeners.cursor_motion);
    wl_signal_add(&cursor->events.motion_absolute, &listeners.cursor_motion_absolute);
    wl_signal_add(&cursor->events.button, &listeners.cursor_button);
    wl_signal_add(&cursor->events.axis, &listeners.cursor_axis);
    wl_signal_add(&cursor->events.frame, &listeners.cursor_frame);

    listeners.new_input.notify = EventListeners::new_input_handler;
    wl_signal_add(&backend->events.new_input, &listeners.new_input);
    seat = wlr_seat_create(wl_display, "seat0");
    listeners.request_cursor.notify = EventListeners::seat_request_cursor_handler;
    wl_signal_add(&seat->events.request_set_cursor, &listeners.request_cursor);
}

void Server::new_keyboard(struct wlr_input_device* device)
{
    keyboards.emplace_back(this, device);
}

void Server::new_pointer(struct wlr_input_device* device)
{
    wlr_cursor_attach_input_device(cursor, device);
}

void Server::process_cursor_motion(uint32_t time)
{
    double sx, sy;
    struct wlr_surface* surface = nullptr;
    const View* view = get_surface_under_cursor(cursor->x, cursor->y, surface, sx, sy);
    if (!view && surface) {
        wlr_log(WLR_DEBUG, "poopy");
    }
    if (!view) {
        // set the cursor to default
        wlr_xcursor_manager_set_cursor_image(cursor_manager, "left_ptr", cursor);
    }
    if (surface) {
        bool focus_changed = seat->pointer_state.focused_surface != surface;
        // Gives pointer focus when the cursor enters the surface
        wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
        if (!focus_changed) {
            // the enter event contains coordinates, so we notify on motion only
            // if the focus did not change
            wlr_seat_pointer_notify_motion(seat, time, sx, sy);
        }
    } else {
        // Clear focus so pointer events are not sent to the last entered surface
        wlr_seat_pointer_clear_focus(seat);
    }
}

void Server::focus_view(View* view, wlr_surface* surface)
{
    if (view == nullptr) {
        return;
    }

    auto* prev_surface = seat->keyboard_state.focused_surface;
    if (prev_surface == surface) {
        return; // already focused
    }
    if (prev_surface) {
        // deactivate previous surface
        auto* prev_xdg_surface = wlr_xdg_surface_from_wlr_surface(seat->keyboard_state.focused_surface);
        wlr_xdg_toplevel_set_activated(prev_xdg_surface, false);
    }
    auto* keyboard = wlr_seat_get_keyboard(seat);
    // move the view to the front
    views.splice(views.begin(), views, view->link);
    view->link = views.begin();
    // activate surface
    wlr_xdg_toplevel_set_activated(view->xdg_surface, true);
    // the seat will send keyboard events to the view automatically
    wlr_seat_keyboard_notify_enter(seat, view->xdg_surface->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

View* Server::get_surface_under_cursor(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
{
    for (auto& view : views) {
        if (view.get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return &view;
        }
    }

    return NULL;
}

bool Server::run()
{
    // add UNIX socket to the Wayland display
    const char* socket = wl_display_add_socket_auto(wl_display);
    if (!socket) {
        wlr_backend_destroy(backend);
        return false;
    }

    if (!wlr_backend_start(backend)) {
        wlr_backend_destroy(backend);
        wl_display_destroy(wl_display);
        return false;
    }

    setenv("WAYLAND_DISPLAY", socket, true);

    wlr_log(WLR_INFO, "Running Cardboard on WAYLAND_DISPLAY=%s", socket);
    wl_display_run(wl_display);

    return true;
}

void Server::stop()
{
    wlr_log(WLR_INFO, "Shutting down Cardboard");
    wl_display_destroy_clients(wl_display);
    wl_display_destroy(wl_display);
}
