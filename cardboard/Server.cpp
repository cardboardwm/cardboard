extern "C" {
#include <wayland-server.h>
#include <wlr/backend.h>
#define static
#include <wlr/types/wlr_compositor.h>
#undef static
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_gtk_primary_selection.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include <cardboard/ipc.h>
#include <wlr_cpp_fixes/types/wlr_layer_shell_v1.h>

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

    compositor = wlr_compositor_create(wl_display, renderer);
    wlr_data_device_manager_create(wl_display); // for clipboard managers

    output_layout = wlr_output_layout_create();

    // https://drewdevault.com/2018/07/29/Wayland-shells.html
    // TODO: implement Xwayland
    xdg_shell = wlr_xdg_shell_create(wl_display);
    layer_shell = wlr_layer_shell_v1_create(wl_display);

    // low effort protocol implementations
    wlr_xdg_output_manager_v1_create(wl_display, output_layout);
    wlr_gamma_control_manager_v1_create(wl_display);
    wlr_export_dmabuf_manager_v1_create(wl_display);
    wlr_screencopy_manager_v1_create(wl_display);
    wlr_data_control_manager_v1_create(wl_display);
    wlr_gtk_primary_selection_device_manager_create(wl_display);
    wlr_primary_selection_v1_device_manager_create(wl_display);

    // less low effort protocol implementations
    inhibit_manager = wlr_input_inhibit_manager_create(wl_display);

    init_seat(this, &seat, DEFAULT_SEAT);

    config = Config {
        .mouse_mods = WLR_MODIFIER_LOGO,
    };

    for (int i = 0; i < WORKSPACE_NR; i++) {
        workspaces.push_back(Workspace(i));
        workspaces.back().set_output_layout(output_layout);
    }

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &backend->events.new_input, Server::new_input_handler },
        { &backend->events.new_output, Server::new_output_handler },

        { &output_layout->events.add, Server::output_layout_add_handler },

        { &xdg_shell->events.new_surface, Server::new_xdg_surface_handler },
        { &layer_shell->events.new_surface, Server::new_layer_surface_handler },

        { &inhibit_manager->events.activate, Server::activate_inhibit_handler },
        { &inhibit_manager->events.deactivate, Server::deactivate_inhibit_handler },
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

    char* env_path = getenv(libcardboard::ipc::SOCKET_ENV_VAR);
    if (env_path != nullptr) {
        socket_path = env_path;
    } else {
        std::string display = "wayland-0";
        if (char* env_display = getenv("WAYLAND_DISPLAY")) {
            display = env_display;
        }
        socket_path = "/tmp/cardboard-" + display;
    }

    ipc = create_ipc(this, socket_path, [this](const CommandData& command_data) -> std::string {
              return dispatch_command(command_data)(this).message;
          }).value();

    return true;
}

#if HAVE_XWAYLAND
void Server::init_xwayland()
{
    wlr_log(WLR_DEBUG, "Initializing Xwayland");
    xwayland = wlr_xwayland_create(wl_display, compositor, true);

    listeners.add_listener(&xwayland->events.new_surface,
                           Listener { new_xwayland_surface_handler, this, NoneT {} });

    setenv("DISPLAY", xwayland->display_name, true);
}
#endif

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
    const auto* wlr_output = wlr_output_layout_output_at(output_layout, lx, ly);
    const auto ws_it = std::find_if(workspaces.begin(), workspaces.end(), [wlr_output](const auto& other) {
        return other.output && other.output.unwrap().wlr_output == wlr_output;
    });
    if (ws_it == workspaces.end() || !ws_it->output) {
        return nullptr;
    }

    // we are trying surfaces from top to bottom

    // first, overlays and top layers
    for (const auto layer : { ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, ZWLR_LAYER_SHELL_V1_LAYER_TOP }) {
        // fullscreen views render on top of the TOP layer
        if (ws_it->fullscreen_view && layer == ZWLR_LAYER_SHELL_V1_LAYER_TOP) {
            continue;
        }
        for (const auto& layer_surface : layers[layer]) {
            if (!layer_surface.surface->mapped || !layer_surface.is_on_output(ws_it->output.raw_pointer())) {
                continue;
            }

            if (layer_surface.get_surface_under_coords(lx, ly, surface, sx, sy)) {
                return nullptr;
            }
        }
    }

    // second, unmanaged xwayland surfaces
#if HAVE_XWAYLAND
    for (const auto xwayland_or_surface : xwayland_or_surfaces) {
        if (xwayland_or_surface->get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return nullptr;
        }
    }
#endif

    if (ws_it->fullscreen_view && ws_it->fullscreen_view.unwrap().get_surface_under_coords(lx, ly, surface, sx, sy)) {
        return ws_it->fullscreen_view.raw_pointer();
    }

    // third, floating views
    for (auto* floating_view : ws_it->floating_views) {
        if (!floating_view->mapped) {
            continue;
        }

        if (floating_view->get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return floating_view;
        }
    }

    // fourth, regular, tiled views
    for (auto& tile : ws_it->tiles) {
        auto* view = tile.view;
        if (!view->mapped) {
            continue;
        }

        if (view->get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return view;
        }
    }

    // and the very last, bottom layers and backgrounds
    for (const auto layer : { ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND }) {
        for (const auto& layer_surface : layers[layer]) {
            if (!layer_surface.surface->mapped) {
                continue;
            }

            if (layer_surface.get_surface_under_coords(lx, ly, surface, sx, sy)) {
                return nullptr;
            }
        }
    }

    return nullptr;
}

void Server::map_view(View* view)
{
    view->mapped = true;

    auto* prev_focused = seat.get_focused_view();

    seat.get_focused_workspace(this).and_then([view, prev_focused](auto& ws) {
        ws.add_view(view, prev_focused);
    });
    seat.focus_view(this, view);
}

void Server::unmap_view(View* view)
{
    if (view->mapped) {
        view->mapped = false;
        get_views_workspace(view).remove_view(view);
    }

    seat.hide_view(this, view);
    seat.remove_from_focus_stack(view);
}

void Server::move_view_to_front(View* view)
{
    views.splice(views.begin(), views, std::find_if(views.begin(), views.end(), [view](const auto x) { return view == x; }));
}

Workspace& Server::get_views_workspace(NotNullPointer<View> view)
{
    assert(view->workspace_id >= 0);
    return workspaces[view->workspace_id];
}

Workspace& Server::create_workspace()
{
    workspaces.push_back(workspaces.size());
    return workspaces.back();
}

bool Server::run()
{
#if HAVE_XWAYLAND
    init_xwayland();
#endif
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

    if (!init_ipc() || !load_settings()) {
        return false;
    }

    wlr_log(WLR_INFO, "Running Cardboard on WAYLAND_DISPLAY=%s", socket);
    wl_display_run(wl_display);

    return true;
}

void Server::stop()
{
    ipc = nullptr; // release ipc system
    wlr_log(WLR_INFO, "Shutting down Cardboard");
#if HAVE_XWAYLAND
    wlr_xwayland_destroy(xwayland);
#endif
    wl_display_destroy_clients(wl_display);
    wl_display_destroy(wl_display);
}

void Server::teardown(int code)
{
    wl_display_terminate(wl_display);
    exit_code = code;
}

void Server::new_output_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* output = static_cast<struct wlr_output*>(data);

    // pick the monitor's preferred mode
    if (!wl_list_empty(&output->modes)) {
        struct wlr_output_mode* mode = wlr_output_preferred_mode(output);
        wlr_output_set_mode(output, mode);
        wlr_output_enable(output, true);
        if (!wlr_output_commit(output)) {
            return;
        }
    }

    // add output to the layout. add_auto arranges outputs left-to-right
    // in the order they appear.
    wlr_output_layout_add_auto(server->output_layout, output);
}

void Server::output_layout_add_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* l_output = static_cast<struct wlr_output_layout_output*>(data);

    auto output_ = Output {};
    output_.wlr_output = l_output->output;
    wlr_output_effective_resolution(output_.wlr_output, &output_.usable_area.width, &output_.usable_area.height);
    register_output(server, std::move(output_));

    auto& output = server->outputs.back();

    Workspace* ws_to_assign = nullptr;
    for (auto& ws : server->workspaces) {
        if (!ws.output) {
            ws_to_assign = &ws;
            break;
        }
    }

    if (!ws_to_assign) {
        ws_to_assign = &server->create_workspace();
    }

    ws_to_assign->activate(output);

    // the output doesn't need to be exposed as a wayland global
    // because wlr_output_layout does it for us already
}

void Server::new_xdg_surface_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* xdg_surface = static_cast<struct wlr_xdg_surface*>(data);

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    create_view(server, new XDGView(xdg_surface));
}

void Server::new_layer_surface_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* layer_surface = static_cast<struct wlr_layer_surface_v1*>(data);
    wlr_log(WLR_DEBUG,
            "new layer surface: namespace %s layer %d anchor %d "
            "size %dx%d margin %d,%d,%d,%d",
            layer_surface->namespace_,
            layer_surface->client_pending.layer,
            layer_surface->client_pending.anchor,
            layer_surface->client_pending.desired_width,
            layer_surface->client_pending.desired_height,
            layer_surface->client_pending.margin.top,
            layer_surface->client_pending.margin.right,
            layer_surface->client_pending.margin.bottom,
            layer_surface->client_pending.margin.left);

    OptionalRef<Output> output_to_assign;
    if (layer_surface->output) {
        output_to_assign = OptionalRef(static_cast<Output*>(layer_surface->output->data));
    } else {
        // Assigns output of the focused workspace
        output_to_assign = server->seat.get_focused_workspace(server)
                               .and_then<Output>([](auto& ws) { return ws.output; })
                               .or_else([server]() {
                                   if (server->outputs.empty()) {
                                       return OptionalRef<Output>(nullptr);
                                   }
                                   return OptionalRef(server->outputs.front());
                               })
                               .or_else([layer_surface]() { wlr_layer_surface_v1_close(layer_surface); return NullRef<Output>; });
    }

    output_to_assign.and_then([output_to_assign, layer_surface, server](auto& output) {
        LayerSurface ls;
        ls.output = output_to_assign;
        ls.surface = layer_surface;
        ls.surface->output = output.wlr_output;
        create_layer(server, std::move(ls));
    });
}

#if HAVE_XWAYLAND
void Server::new_xwayland_surface_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* xsurface = static_cast<struct wlr_xwayland_surface*>(data);

    if (xsurface->override_redirect) {
        create_xwayland_or_surface(server, xsurface);
        return;
    }

    wlr_log(WLR_DEBUG, "new xwayland surface title='%s' class='%s'", xsurface->title, xsurface->class_);
    create_view(server, new XwaylandView(server, xsurface));
}
#endif

void Server::new_input_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* device = static_cast<struct wlr_input_device*>(data);

    server->seat.add_input_device(server, device);
}

void Server::activate_inhibit_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);

    server->seat.set_exclusive_client(server, server->inhibit_manager->active_client);
}

void Server::deactivate_inhibit_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);

    server->seat.set_exclusive_client(server, nullptr);
}
