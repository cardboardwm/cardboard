#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_compositor.h>
#include <wlr_cpp/types/wlr_cursor.h>
#include <wlr_cpp/types/wlr_data_device.h>
#include <wlr_cpp/types/wlr_output_layout.h>
#include <wlr_cpp/types/wlr_xcursor_manager.h>
#include <wlr_cpp/util/log.h>

#include <cassert>

#include "Server.h"

Server::Server()
{
    wl_display = wl_display_create();
    // let wlroots select the required hardware abstractions
    backend = wlr_backend_autocreate(wl_display, nullptr);

    renderer = wlr_backend_get_renderer(backend);
    wlr_renderer_init_wl_display(renderer, wl_display);

    wlr_compositor_create(wl_display, renderer);
    wlr_data_device_manager_create(wl_display); // for clipboard

    output_layout = wlr_output_layout_create();

    // https://drewdevault.com/2018/07/29/Wayland-shells.html
    // TODO: implement layer-shell
    // TODO: implement Xwayland
    xdg_shell = wlr_xdg_shell_create(wl_display);

    cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(cursor, output_layout);

    // the cursor manager loads xcursor images for all scale factors
    cursor_manager = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(cursor_manager, 1);

    seat = wlr_seat_create(wl_display, "seat0");

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &cursor->events.motion, cursor_motion_handler },
        { &cursor->events.motion_absolute, cursor_motion_absolute_handler },
        { &cursor->events.button, cursor_button_handler },
        { &cursor->events.axis, cursor_axis_handler },
        { &cursor->events.frame, cursor_frame_handler },
        { &backend->events.new_output, new_output_handler },
        { &xdg_shell->events.new_surface, new_xdg_surface_handler },
        { &backend->events.new_input, new_input_handler },
        { &seat->events.request_set_cursor, seat_request_cursor_handler }
    };

    for (const auto& to_add_listener : to_add_listeners) {
        listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, this, NoneT {} });
    }
}

void Server::new_keyboard(struct wlr_input_device* device)
{
    keyboards.push_back(device);

    struct xkb_rule_names rules = {};
    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    listeners.add_listener(&device->keyboard->events.modifiers, Listener { modifiers_handler, this, device });
    listeners.add_listener(&device->keyboard->events.key, Listener { key_handler, this, device });
    wlr_seat_set_keyboard(seat, device);
}

void Server::new_pointer(struct wlr_input_device* device)
{
    wlr_cursor_attach_input_device(cursor, device);
}

void Server::process_cursor_motion(uint32_t time)
{
    if (grab_state.has_value() && grab_state->mode == GrabState::Mode::MOVE) {
        process_cursor_move();
        return;
    }
    if (grab_state.has_value() && grab_state->mode == GrabState::Mode::RESIZE) {
        process_cursor_resize();
        return;
    }
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

void Server::process_cursor_move()
{
    assert(grab_state.has_value());
    grab_state->view->x = cursor->x - grab_state->x;
    grab_state->view->y = cursor->y - grab_state->y;
}

void Server::process_cursor_resize()
{
    assert(grab_state.has_value());
    double dx = cursor->x - grab_state->x;
    double dy = cursor->y - grab_state->y;
    double x = grab_state->view->x;
    double y = grab_state->view->y;
    double width = grab_state->width;
    double height = grab_state->height;
    if (grab_state->resize_edges & WLR_EDGE_TOP) {
        y = grab_state->y + dy;
        height -= dy;
        if (height < 1) {
            y += height;
        }
    } else if (grab_state->resize_edges & WLR_EDGE_BOTTOM) {
        height += dy;
    }
    if (grab_state->resize_edges & WLR_EDGE_LEFT) {
        x = grab_state->x + dx;
        width -= dx;
        if (width < 1) {
            x += width;
        }
    } else if (grab_state->resize_edges & WLR_EDGE_RIGHT) {
        width += dx;
    }
    grab_state->view->x = x;
    grab_state->view->y = y;
    wlr_xdg_toplevel_set_size(grab_state->view->xdg_surface, width, height);
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
    views.splice(views.begin(), views, std::find_if(views.begin(), views.end(), [view](auto& x) { return view == &x; }));
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

void Server::begin_interactive(View* view, GrabState::Mode mode, uint32_t edges)
{
    assert(grab_state == std::nullopt);
    struct wlr_surface* focused_surface = seat->pointer_state.focused_surface;
    if (view->xdg_surface->surface != focused_surface) {
        // don't handle the request if the view is not in focus
        return;
    }
    struct wlr_box geometry;
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geometry);

    GrabState state = { mode, view, 0, 0, geometry.width, geometry.height, edges };
    if (mode == GrabState::Mode::MOVE) {
        state.x = cursor->x - view->x;
        state.y = cursor->y - view->y;
    } else {
        state.x = cursor->x;
        state.y = cursor->y;
    }
    grab_state = state;
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
