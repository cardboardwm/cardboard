#include "SurfaceManager.h"
#include "Server.h"

void SurfaceManager::map_view(Server& server, View& view)
{
    view.mapped = true;

    // can be nullptr, this is fine
    auto* prev_focused = server.seat.get_focused_view().raw_pointer();

    server.seat.get_focused_workspace(server).and_then([&server, &view, prev_focused](auto& ws) {
        ws.add_view(server.output_manager, view, prev_focused);
    });
    server.seat.focus_view(server, view);
}

void SurfaceManager::unmap_view(Server& server, View& view)
{
    if (view.mapped) {
        view.mapped = false;
        server.output_manager.get_view_workspace(view).remove_view(server.output_manager, view);
    }

    server.seat.hide_view(server, view);
    server.seat.remove_from_focus_stack(view);
}

void SurfaceManager::move_view_to_front(View& view)
{
    views.splice(views.begin(), views, std::find_if(views.begin(), views.end(), [&view](const std::unique_ptr<View>& x) { return &view == x.get(); }));
}

OptionalRef<View> SurfaceManager::get_surface_under_cursor(OutputManager& output_manager, double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy)
{
    OptionalRef<Output> output = output_manager.get_output_at(lx, ly);
    const auto ws_it = std::find_if(output_manager.workspaces.begin(), output_manager.workspaces.end(), [output](const auto& other) {
        return other.output == output;
    });
    if (ws_it == output_manager.workspaces.end() || !ws_it->output) {
        return NullRef<View>;
    }

    // it is guaranteed that the workspace is activated on an output

    // we are trying surfaces from top to bottom

    // first, overlays and top layers
    for (const auto layer : { ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, ZWLR_LAYER_SHELL_V1_LAYER_TOP }) {
        // fullscreen views render on top of the TOP layer
        if (ws_it->fullscreen_view && layer == ZWLR_LAYER_SHELL_V1_LAYER_TOP) {
            continue;
        }
        for (const auto& layer_surface : layers[layer]) {
            if (!layer_surface.surface->mapped || !layer_surface.is_on_output(ws_it->output.unwrap())) {
                continue;
            }

            if (layer_surface.get_surface_under_coords(lx, ly, surface, sx, sy)) {
                return NullRef<View>;
            }
        }
    }

    // second, unmanaged xwayland surfaces
#if HAVE_XWAYLAND
    for (const auto& xwayland_or_surface : xwayland_or_surfaces) {
        if (!xwayland_or_surface->mapped) {
            continue;
        }
        if (xwayland_or_surface->get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return NullRef<View>;
        }
    }
#endif

    if (ws_it->fullscreen_view && ws_it->fullscreen_view.unwrap().get_surface_under_coords(lx, ly, surface, sx, sy)) {
        return ws_it->fullscreen_view;
    }

    // third, floating views
    for (NotNullPointer<View> floating_view : ws_it->floating_views) {
        if (!floating_view->mapped) {
            continue;
        }

        if (floating_view->get_surface_under_coords(lx, ly, surface, sx, sy)) {
            return OptionalRef(floating_view);
        }
    }

    // fourth, regular, tiled views
    for (auto& column : ws_it->columns) {
        for (auto& tile : column.tiles) {
            NotNullPointer<View> view = tile.view;
            if (!view->mapped) {
                continue;
            }

            if (view->get_surface_under_coords(lx, ly, surface, sx, sy)) {
                return OptionalRef(view);
            }
        }
    }

    // and the very last, bottom layers and backgrounds
    for (const auto layer : { ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND }) {
        for (const auto& layer_surface : layers[layer]) {
            if (!layer_surface.surface->mapped) {
                continue;
            }

            if (layer_surface.get_surface_under_coords(lx, ly, surface, sx, sy)) {
                return NullRef<View>;
            }
        }
    }

    return NullRef<View>;
}
