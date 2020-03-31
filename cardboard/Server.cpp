#include <cardboard_common/IPC.h>
#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_compositor.h>
#include <wlr_cpp/types/wlr_cursor.h>
#include <wlr_cpp/types/wlr_data_device.h>
#include <wlr_cpp/types/wlr_output_layout.h>
#include <wlr_cpp/types/wlr_xcursor_manager.h>
#include <wlr_cpp/util/log.h>

#include <sys/socket.h>

#include <cassert>

#include "IPC.h"
#include "Server.h"
#include "Spawn.h"

bool Server::init()
{
    wl_display = wl_display_create();
    // let wlroots select the required hardware abstractions
    backend = wlr_backend_autocreate(wl_display, nullptr);

    event_loop = wl_display_get_event_loop(wl_display);

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

    for (int i = 0; i < WORKSPACE_NR; i++) {
        workspaces.push_back(Workspace(i));
        workspaces.back().set_output_layout(output_layout);
    }

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
        { &output_layout->events.add, output_layout_add_handler },
        { &xdg_shell->events.new_surface, new_xdg_surface_handler },
        { &backend->events.new_input, new_input_handler },
        { &seat->events.request_set_cursor, seat_request_cursor_handler }
    };

    for (const auto& to_add_listener : to_add_listeners) {
        listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, this, NoneT {} });
    }

    return true;
}

bool Server::init_ipc()
{
    std::string socket_path;

    char* env_path = getenv(IPC_SOCKET_ENV_VAR);
    if (env_path != nullptr) {
        socket_path = env_path;
    } else {
        std::string display = "wayland-0";
        if (char* env_display = getenv("WAYLAND_DISPLAY")) {
            display = env_display;
        }
        socket_path = "/tmp/cardboard-" + display;
    }

    ipc_sock_address.sun_family = AF_UNIX;
    strncpy(ipc_sock_address.sun_path, socket_path.c_str(), sizeof(ipc_sock_address.sun_path));

    ipc_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ipc_socket_fd == -1) {
        wlr_log(WLR_ERROR, "Couldn't create the IPC socket.");
        return false;
    }

    unlink(socket_path.c_str());

    if (bind(ipc_socket_fd, reinterpret_cast<sockaddr*>(&ipc_sock_address), sizeof(ipc_sock_address)) == -1) {
        wlr_log(WLR_ERROR, "Couldn't bind a name ('%s') to the IPC socket.", ipc_sock_address.sun_path);
        return false;
    }

    if (listen(ipc_socket_fd, SOMAXCONN) == -1) {
        wlr_log(WLR_ERROR, "Couldn't listen to the IPC socket '%s'.", ipc_sock_address.sun_path);
        return false;
    }

    setenv(IPC_SOCKET_ENV_VAR, ipc_sock_address.sun_path, 0);
    ipc_event_source = wl_event_loop_add_fd(event_loop, ipc_socket_fd, WL_EVENT_READABLE, ipc_read_command, this);

    wlr_log(WLR_INFO, "Listening on socket %s", ipc_sock_address.sun_path);

    return true;
}

bool Server::load_settings()
{
    if (const char* config_home = getenv(CONFIG_HOME_ENV.data()); config_home != nullptr) {
        // please std::format end my suffering
        config_path += config_home;
        config_path += '/';
        config_path += CARDBOARD_NAME;
        config_path += '/';
        config_path += CONFIG_NAME;
    } else {
        const char* home = getenv("HOME");
        if (home == nullptr) {
            wlr_log(WLR_ERROR, "Couldn't get home directory");
            return false;
        }

        config_path += home;
        config_path += "/.config/";
        config_path += CARDBOARD_NAME;
        config_path += '/';
        config_path += CONFIG_NAME;
    }

    wlr_log(WLR_DEBUG, "Running config file %s", config_path.c_str());

    auto error_code = spawn([&]() {
        execle(config_path.c_str(), config_path.c_str(), nullptr, environ);

        return EXIT_FAILURE;
    });
    if (error_code.value() != 0) {
        wlr_log(WLR_ERROR, "Couldn't execute the config file: %s", error_code.message().c_str());
        return false;
    }

    return true;
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
    listeners.add_listener(
        &device->keyboard->events.key,
        Listener { key_handler, this, KeyHandleData { device, &keybindings_config } });
    listeners.add_listener(&device->keyboard->events.destroy, Listener { keyboard_destroy_handler, this, device });
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
    grab_state->view->resize(width, height);

    // TODO: fix resizing
    if (auto ws = get_views_workspace(grab_state->view)) {
        ws->get().arrange_tiles();
    }
}

void Server::focus_view(View* view)
{
    if (view == nullptr) {
        return;
    }

    View* prev_view = get_focused_view();
    if (prev_view == view) {
        return; // already focused
    }
    if (prev_view) {
        // deactivate previous surface
        prev_view->set_activated(false);
    }

    if (auto it = std::find(focused_views.begin(), focused_views.end(), view); it != focused_views.end()) {
        focused_views.splice(focused_views.begin(), focused_views, it);
    } else {
        focused_views.push_front(view);
    }

    auto* keyboard = wlr_seat_get_keyboard(seat);
    // move the view to the front
    views.splice(views.begin(), views, std::find_if(views.begin(), views.end(), [view](const auto x) { return view == x; }));
    // activate surface
    view->set_activated(true);
    // the seat will send keyboard events to the view automatically
    wlr_seat_keyboard_notify_enter(seat, view->get_surface(), keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);

    if (auto ws = get_views_workspace(view)) {
        ws->get().fit_view_on_screen(view);
    }
}

void Server::focus_by_offset(int offset)
{
    if (offset == 0 || get_focused_view() == nullptr) {
        return;
    }

    auto* focused_view = get_focused_view();
    auto ws = get_views_workspace(focused_view);
    if (!ws) {
        return;
    }

    auto it = ws->get().find_tile(focused_view);
    if (int index = std::distance(ws->get().tiles.begin(), it) + offset; index < 0 || index >= static_cast<int>(ws->get().tiles.size())) {
        // out of bounds
        return;
    }

    std::advance(it, offset);
    focus_view(it->view);
}

void Server::hide_view(View* view)
{
    focused_views.remove_if([view](auto other) { return other == view; });

    // focus last focused window mapped to an active workspace
    if (get_focused_view() == view && !focused_views.empty()) {
        View* to_focus = nullptr;
        for (View* v : focused_views) {
            if (auto ws = get_views_workspace(v); ws && ws->get().output) {
                to_focus = v;
                break;
            }
        }

        if (to_focus != nullptr) {
            focus_view(to_focus);
        }
    }
}

View* Server::get_surface_under_cursor(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
{
    auto output = wlr_output_layout_output_at(output_layout, lx, ly);
    for (const auto view : views) {
        auto views_output = get_views_workspace(view)->get().output;
        // the view is either tiled in the output holding the cursor, or not tiled at all
        if (((views_output && *views_output == output) || !views_output)
            && view->get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return view;
        }
    }

    return nullptr;
}

View* Server::get_focused_view()
{
    for (const auto view : views) {
        if (view->get_surface() == seat->keyboard_state.focused_surface) {
            return view;
        }
    }

    return nullptr;
}

void Server::map_view(View* view)
{
    view->mapped = true;

    auto* prev_focused = get_focused_view();

    Workspace& focused_workspace = get_focused_workspace();
    focused_workspace.add_view(view, prev_focused);
    focus_view(view);
}

void Server::unmap_view(View* view)
{
    view->mapped = false;
    if (auto ws = get_views_workspace(view)) {
        ws->get().remove_view(view);
    }

    hide_view(view);
}

std::optional<std::reference_wrapper<Workspace>> Server::get_views_workspace(View* view)
{
    if (view->workspace_id < 0) {
        return std::nullopt;
    }

    return std::ref(workspaces[view->workspace_id]);
}

Workspace& Server::get_focused_workspace()
{
    for (auto& ws : workspaces) {
        if (ws.output && wlr_output_layout_contains_point(output_layout, *ws.output, cursor->x, cursor->y)) {
            return ws;
        }
    }

    assert(false);
}

Workspace& Server::create_workspace()
{
    workspaces.push_back(workspaces.size());
    return workspaces.back();
}

void Server::begin_interactive(View* view, GrabState::Mode mode, uint32_t edges)
{
    assert(grab_state == std::nullopt);
    struct wlr_surface* focused_surface = seat->pointer_state.focused_surface;
    if (view->get_surface() != focused_surface) {
        // don't handle the request if the view is not in focus
        return;
    }
    struct wlr_box geometry = view->geometry;

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

    if(!init_ipc() || !load_settings()){
        return false;
    }

    wlr_log(WLR_INFO, "Running Cardboard on WAYLAND_DISPLAY=%s", socket);
    wl_display_run(wl_display);

    return true;
}

void Server::stop()
{
    wlr_log(WLR_INFO, "Shutting down Cardboard");
    wl_display_destroy_clients(wl_display);
    wl_display_destroy(wl_display);
    close(ipc_socket_fd);
}

void Server::teardown(int code)
{
    wl_display_terminate(wl_display);
    exit_code = code;
}
