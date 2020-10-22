// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_LAYERS_H_INCLUDED
#define CARDBOARD_LAYERS_H_INCLUDED

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_surface.h>

#include <wlr_cpp_fixes/types/wlr_layer_shell_v1.h>
}

#include <array>
#include <list>

#include "NotNull.h"
#include "OptionalRef.h"

struct Server;
struct Output;
struct OutputManager;

/**
 * \brief Represents a layer_surface from the layer shell in the compositor.
 */
struct LayerSurface {
    struct wlr_layer_surface_v1* surface;
    struct wlr_box geometry;
    enum zwlr_layer_shell_v1_layer layer;
    OptionalRef<Output> output;

    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy) const;
    /// Returns true if \a output is the output of this layer surface.
    bool is_on_output(Output& output) const;

    static void commit_handler(struct wl_listener* listener, void* data);
    static void destroy_handler(struct wl_listener* listener, void* data);
    static void map_handler(struct wl_listener* listener, void* data);
    static void unmap_handler(struct wl_listener* listener, void* data);
    static void new_popup_handler(struct wl_listener* listener, void* data);
    static void output_destroy_handler(struct wl_listener* listener, void* data);
};

struct LayerSurfacePopup {
    struct wlr_xdg_popup* wlr_popup;
    NotNullPointer<LayerSurface> parent;

    void unconstrain(OutputManager& output_manager);

    static void destroy_handler(struct wl_listener* listener, void* data);
    static void new_popup_handler(struct wl_listener* listener, void* data);
    static void map_handler(struct wl_listener* listener, void* data);
};

using LayerArray = std::array<std::list<LayerSurface>, 4>;

/// Registers a LayerSurface.
void create_layer(Server& server, LayerSurface&& layer_surface);

/// Arranges all the layers of an \a output.
void arrange_layers(Server& server, Output& output);

#endif // CARDBOARD_LAYERS_H_INCLUDED
