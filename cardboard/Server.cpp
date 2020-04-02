#include <cardboard_common/IPC.h>
#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/types/wlr_compositor.h>
#include <wlr_cpp/types/wlr_cursor.h>
#include <wlr_cpp/types/wlr_data_device.h>
#include <wlr_cpp/types/wlr_output_layout.h>
#include <wlr_cpp/types/wlr_xcursor_manager.h>
#include <wlr_cpp/types/wlr_xdg_output_v1.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>
#include <wlr_cpp/util/log.h>

#include <layer_shell_v1.h>

#include <sys/socket.h>

#include <cassert>

#include "IPC.h"
#include "Seat.h"
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
    // TODO: implement Xwayland
    xdg_shell = wlr_xdg_shell_create(wl_display);
    layer_shell = wlr_layer_shell_v1_create(wl_display);

    wlr_xdg_output_manager_v1_create(wl_display, output_layout);

    init_seat(this, &seat, DEFAULT_SEAT);

    for (int i = 0; i < WORKSPACE_NR; i++) {
        workspaces.push_back(Workspace(i));
        workspaces.back().set_output_layout(output_layout);
    }

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &backend->events.new_output, new_output_handler },
        { &output_layout->events.add, output_layout_add_handler },

        { &xdg_shell->events.new_surface, new_xdg_surface_handler },
        { &layer_shell->events.new_surface, new_layer_surface_handler },

        { &backend->events.new_input, new_input_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        listeners.add_listener(
            to_add_listener.signal,
            Listener { to_add_listener.notify, this, NoneT {} });
    }

    return true;
}

bool Server::init_ipc1()
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

    return true;
}

void Server::init_ipc2()
{
    ipc_event_source = wl_event_loop_add_fd(event_loop, ipc_socket_fd, WL_EVENT_READABLE, ipc_read_command, this);

    wlr_log(WLR_INFO, "Listening on socket %s", ipc_sock_address.sun_path);
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

View* Server::get_surface_under_cursor(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
{
    auto wlr_output = wlr_output_layout_output_at(output_layout, lx, ly);
    for (const auto view : views) {
        auto views_output = get_views_workspace(view)->get().output;
        // the view is either tiled in the output holding the cursor, or not tiled at all
        if (((views_output && (*views_output)->wlr_output == wlr_output) || !views_output)
            && view->get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return view;
        }
    }

    return nullptr;
}

void Server::map_view(View* view)
{
    view->mapped = true;

    auto* prev_focused = seat.get_focused_view();

    if (auto focused_workspace = seat.get_focused_workspace(this); focused_workspace) {
        focused_workspace->get().add_view(view, prev_focused);
    }
    seat.focus_view(this, view);
}

void Server::unmap_view(View* view)
{
    view->mapped = false;
    if (auto ws = get_views_workspace(view)) {
        ws->get().remove_view(view);
    }

    seat.hide_view(this, view);
    seat.remove_from_focus_stack(view);
}

void Server::move_view_to_front(View* view)
{
    views.splice(views.begin(), views, std::find_if(views.begin(), views.end(), [view](const auto x) { return view == x; }));
}

std::optional<std::reference_wrapper<Workspace>> Server::get_views_workspace(View* view)
{
    if (view->workspace_id < 0) {
        return std::nullopt;
    }

    return std::ref(workspaces[view->workspace_id]);
}

Workspace& Server::create_workspace()
{
    workspaces.push_back(workspaces.size());
    return workspaces.back();
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
    close(ipc_socket_fd);
}

void Server::teardown(int code)
{
    wl_display_terminate(wl_display);
    exit_code = code;
}
