// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
extern "C" {
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>
}

#include <wlr_cpp_fixes/types/wlr_layer_shell_v1.h>

#include <cstring>

#include "Helpers.h"
#include "Layers.h"
#include "Listener.h"
#include "Server.h"

/// Creates and registers a LayerSurfacePopup whose parent is \a layer_surface.
static void create_layer_popup(Server& server, NotNullPointer<struct wlr_xdg_popup> wlr_popup, NotNullPointer<LayerSurface> layer_surface)
{
    wlr_log(WLR_DEBUG, "new layer popup");
    auto* popup = new LayerSurfacePopup { wlr_popup, layer_surface };

    register_handlers(server,
                      popup,
                      {
                          { &popup->wlr_popup->base->events.destroy, &LayerSurfacePopup::destroy_handler },
                          { &popup->wlr_popup->base->events.new_popup, &LayerSurfacePopup::new_popup_handler },
                          { &popup->wlr_popup->base->events.map, &LayerSurfacePopup::map_handler },
                      });

    popup->unconstrain(*(server.output_manager));
}

void create_layer(Server& server, LayerSurface&& layer_surface_)
{
    auto layer = layer_surface_.surface->client_pending.layer;
    server.surface_manager.layers[layer].push_back(layer_surface_);
    auto& layer_surface = server.surface_manager.layers[layer].back();
    layer_surface.layer = layer;
    layer_surface.surface->data = &layer_surface;

    register_handlers(server, &layer_surface, {
                                                  { &layer_surface.surface->surface->events.commit, LayerSurface::commit_handler },
                                                  { &layer_surface.surface->events.destroy, LayerSurface::destroy_handler },
                                                  { &layer_surface.surface->events.map, LayerSurface::map_handler },
                                                  { &layer_surface.surface->events.unmap, LayerSurface::unmap_handler },
                                                  { &layer_surface.surface->events.new_popup, LayerSurface::new_popup_handler },
                                                  { &layer_surface.surface->output->events.destroy, LayerSurface::output_destroy_handler },
                                              });

    // Set the layer's current state to client_pending
    // to easily arrange it.
    auto old_state = layer_surface.surface->current;
    layer_surface.surface->current = layer_surface.surface->client_pending;
    arrange_layers(server, layer_surface.output.unwrap());
    layer_surface.surface->current = old_state;
}

bool LayerSurface::get_surface_under_coords(double lx, double ly, struct wlr_surface*& surf, double& sx, double& sy) const
{
    double layer_x = lx - geometry.x;
    double layer_y = ly - geometry.y;

    double sx_, sy_;
    struct wlr_surface* surface_ = nullptr;
    surface_ = wlr_layer_surface_v1_surface_at(this->surface, layer_x, layer_y, &sx_, &sy_);

    if (surface_ != nullptr) {
        sx = sx_;
        sy = sy_;
        surf = surface_;
        return true;
    }

    return false;
}

bool LayerSurface::is_on_output(Output& out) const
{
    return output && &output.unwrap() == &out;
}

void LayerSurfacePopup::unconstrain(OutputManager& output_manager)
{
    auto* output = static_cast<Output*>(parent->surface->output->data);
    auto* output_box = wlr_output_layout_get_box(output_manager.output_layout, output->wlr_output);

    struct wlr_box output_toplevel_sx_box = {
        .x = -parent->geometry.x,
        .y = -parent->geometry.y,
        .width = output_box->width,
        .height = output_box->height,
    };

    wlr_xdg_popup_unconstrain_from_box(wlr_popup, &output_toplevel_sx_box);
}

static void apply_exclusive_zone(struct wlr_box* usable_area, const wlr_layer_surface_v1_state* state)
{
    uint32_t anchor = state->anchor;
    int32_t exclusive = state->exclusive_zone;
    int32_t margin_top = state->margin.top;
    int32_t margin_right = state->margin.right;
    int32_t margin_bottom = state->margin.bottom;
    int32_t margin_left = state->margin.left;
    if (exclusive <= 0) {
        return;
    }

    struct {
        uint32_t anchors;
        int* positive_axis;
        int* negative_axis;
        int margin;
    } edges[] = {
        {
            .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
            .positive_axis = &usable_area->y,
            .negative_axis = &usable_area->height,
            .margin = margin_top,
        },
        {
            .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
            .positive_axis = nullptr,
            .negative_axis = &usable_area->height,
            .margin = margin_bottom,
        },
        {
            .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
            .positive_axis = &usable_area->x,
            .negative_axis = &usable_area->width,
            .margin = margin_left,
        },
        {
            .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
            .positive_axis = nullptr,
            .negative_axis = &usable_area->width,
            .margin = margin_right,
        },
    };

    for (auto& edge : edges) {
        if ((anchor & edge.anchors) == edge.anchors && exclusive + edge.margin > 0) {
            if (edge.positive_axis) {
                *edge.positive_axis += exclusive + edge.margin;
            }
            if (edge.negative_axis) {
                *edge.negative_axis -= exclusive + edge.margin;
            }
        }
    }
}

static void arrange_layer(Output& output, LayerArray::value_type& layer_surfaces, NotNullPointer<struct wlr_box> usable_area, bool exclusive)
{
    struct wlr_box full_area = {};
    wlr_output_effective_resolution(output.wlr_output, &full_area.width, &full_area.height);

    for (auto& layer_surface : layer_surfaces) {
        if (!layer_surface.is_on_output(output)) {
            continue;
        }

        const auto* state = &layer_surface.surface->current;
        if (exclusive != (state->exclusive_zone > 0)) {
            continue;
        }

        struct wlr_box bounds;
        if (state->exclusive_zone == -1) {
            bounds = full_area;
        } else {
            bounds = *usable_area;
        }

        struct wlr_box box = {};
        box.width = state->desired_width;
        box.height = state->desired_height;

        // horizontal axis
        const auto both_horiz = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        if ((state->anchor & both_horiz) && box.width == 0) {
            box.x = bounds.x;
            box.width = bounds.width;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) {
            box.x = bounds.x;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) {
            box.x = bounds.x + (bounds.width - box.width);
        } else {
            box.x = bounds.x + (bounds.width / 2 - box.width / 2);
        }

        // vertical axis
        const auto both_vert = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        if ((state->anchor & both_vert) && box.height == 0) {
            box.y = bounds.y;
            box.height = bounds.height;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) {
            box.y = bounds.y;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) {
            box.y = bounds.y + (bounds.height - box.height);
        } else {
            box.y = bounds.y + (bounds.height / 2 - box.height / 2);
        }

        // margin
        if ((state->anchor & both_horiz) == both_horiz) {
            box.x += state->margin.left;
            box.width -= state->margin.left + state->margin.right;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) {
            box.x += state->margin.left;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) {
            box.x -= state->margin.right;
        }
        if ((state->anchor & both_vert) == both_vert) {
            box.y += state->margin.top;
            box.height -= state->margin.top + state->margin.bottom;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) {
            box.y += state->margin.top;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) {
            box.y -= state->margin.bottom;
        }

        if (box.width < 0 || box.height < 0) {
            wlr_layer_surface_v1_close(layer_surface.surface);
            continue;
        }

        layer_surface.geometry = box;
        apply_exclusive_zone(usable_area, state);
        wlr_layer_surface_v1_configure(layer_surface.surface, box.width, box.height);
    }
}

void arrange_layers(Server& server, Output& output)
{
    struct wlr_box usable_area = {};
    wlr_output_effective_resolution(output.wlr_output, &usable_area.width, &usable_area.height);

    // arrange exclusive surfaces from top to bottom
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], &usable_area, true);
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], &usable_area, true);
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], &usable_area, true);
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], &usable_area, true);

    if (memcmp(&usable_area, &output.usable_area, sizeof(struct wlr_box)) != 0) {
        output.usable_area = usable_area;
        auto ws_it = std::find_if(server.output_manager->workspaces.begin(), server.output_manager->workspaces.end(), [&output](const auto& other) { return other.output && other.output.raw_pointer() == &output; });
        assert(ws_it != server.output_manager->workspaces.end());
        wlr_log(WLR_DEBUG, "usable area changed");
        if (auto focused_view = server.seat.get_focused_view(); focused_view.has_value() && focused_view.unwrap().workspace_id == ws_it->index) {
            ws_it->fit_view_on_screen(*(server.output_manager), focused_view.unwrap());
        } else {
            ws_it->arrange_workspace(*(server.output_manager));
        }
    }

    // arrange non-exclusive surfaces from top to bottom
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], &usable_area, false);
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], &usable_area, false);
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], &usable_area, false);
    arrange_layer(output, server.surface_manager.layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], &usable_area, false);

    // finds top-most layer surface, if it exists

    // layers above the views, ordered from closest to the top to least close
    uint32_t layers_above_shell[] = {
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        ZWLR_LAYER_SHELL_V1_LAYER_TOP
    };
    LayerSurface* topmost = nullptr;
    for (const auto layer : layers_above_shell) {
        for (auto& layer_surface : server.surface_manager.layers[layer]) {
            if (layer_surface.surface->current.keyboard_interactive && layer_surface.is_on_output(output) && layer_surface.surface->mapped) {
                topmost = &layer_surface;
                break;
            }
        }
        if (topmost != nullptr) {
            break;
        }
    }

    if (topmost != nullptr) {
        server.seat.focus_layer(server, topmost->surface);
    } else if (server.seat.focused_layer && !(*server.seat.focused_layer)->current.keyboard_interactive) {
        server.seat.focus_layer(server, nullptr);
    }
}

void LayerSurface::commit_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* layer_surface = get_listener_data<LayerSurface*>(listener);

    layer_surface->output.and_then([server](auto& output) {
        arrange_layers(*server, output);
    });

    bool layer_changed = layer_surface->layer != layer_surface->surface->current.layer;
    if (layer_changed) {
        auto& old_layer = server->surface_manager.layers[layer_surface->layer];
        auto& new_layer = server->surface_manager.layers[layer_surface->surface->current.layer];

        auto old_layer_it = std::find_if(old_layer.begin(), old_layer.end(), [layer_surface](const auto& other) { return &other == layer_surface; });
        if (old_layer_it != old_layer.end()) {
            new_layer.splice(old_layer_it, old_layer, new_layer.end());
        }
        layer_surface->layer = layer_surface->surface->current.layer;
    }
    server->output_manager->set_dirty();
}

void LayerSurface::destroy_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* layer_surface = get_listener_data<LayerSurface*>(listener);

    wlr_log(WLR_DEBUG, "destroyed layer surface: namespace %s layer %d", layer_surface->surface->namespace_, layer_surface->surface->current.layer);
    server->listeners.clear_listeners(layer_surface);

    server->surface_manager.layers[layer_surface->layer].remove_if([layer_surface](const auto& other) { return &other == layer_surface; });
    // we arrange in destroy and not in unmap because unmapping is always preceded by a commit event which should take care of it
    layer_surface->output.and_then([server](auto& out) { arrange_layers(*server, out); });
}

void LayerSurface::map_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* layer_surface = get_listener_data<LayerSurface*>(listener);

    wlr_surface_send_enter(layer_surface->surface->surface, layer_surface->surface->output);
    cursor_rebase(*server, server->seat, server->seat.cursor);
}

void LayerSurface::unmap_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* layer_surface = get_listener_data<LayerSurface*>(listener);

    if (server->seat.focused_layer == layer_surface->surface) {
        server->seat.focus_layer(*server, nullptr);
    }
    //server->seat.cursor.rebase(server);
}

void LayerSurface::new_popup_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* layer_surface = get_listener_data<LayerSurface*>(listener);
    auto* wlr_popup = static_cast<struct wlr_xdg_popup*>(data);

    create_layer_popup(*server, wlr_popup, layer_surface);
}

void LayerSurface::output_destroy_handler(struct wl_listener* listener, void*)
{
    auto* layer_surface = get_listener_data<LayerSurface*>(listener);
    auto* server = get_server(listener);

    auto* client = wl_resource_get_client(layer_surface->surface->resource);

    layer_surface->surface->mapped = false;

    // if the layer's client has exclusivity, we must focus the first mapped layer of the client,
    // now that this layer is getting unmapped.
    //
    // so we begin searching for it!
    if (client == server->seat.exclusive_client) {
        LayerSurface* layer_surface_to_focus = nullptr;
        if (client == server->seat.exclusive_client) {
            for (auto& layer : server->surface_manager.layers) {
                for (auto& lf : layer) {
                    if (wl_resource_get_client(lf.surface->resource) == client && lf.surface->mapped) {
                        layer_surface_to_focus = &lf;
                    }
                }
                if (layer_surface_to_focus != nullptr) {
                    break;
                }
            }
            if (layer_surface_to_focus != nullptr) {
                server->seat.focus_layer(*server, layer_surface_to_focus->surface);
            }
        }
    }
    // we don't have to remove the layer surface from the layer list of the output
    // because the list is destroyed either way in Output.cpp:output_destroy_handler
    layer_surface->surface->output = nullptr;
    layer_surface->output = NullRef<Output>;
    wlr_layer_surface_v1_close(layer_surface->surface);
}

void LayerSurfacePopup::destroy_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* popup = get_listener_data<LayerSurfacePopup*>(listener);

    server->listeners.clear_listeners(popup);
    delete popup;
}

void LayerSurfacePopup::new_popup_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* popup = get_listener_data<LayerSurfacePopup*>(listener);
    auto* wlr_popup = static_cast<struct wlr_xdg_popup*>(data);

    create_layer_popup(*server, wlr_popup, popup->parent);
}

void LayerSurfacePopup::map_handler(struct wl_listener* listener, void*)
{
    auto* popup = get_listener_data<LayerSurfacePopup*>(listener);

    wlr_surface_send_enter(popup->wlr_popup->base->surface, popup->parent->surface->output);
}
