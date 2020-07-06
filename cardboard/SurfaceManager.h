#ifndef CARDBOARD_VIEW_MANAGER_H_INCLUDED
#define CARDBOARD_VIEW_MANAGER_H_INCLUDED

#include "BuildConfig.h"

#include <list>
#include <memory>

#include "Layers.h"
#include "OptionalRef.h"
#include "View.h"
#include "Workspace.h"
#include "Xwayland.h"

struct SurfaceManager {
    std::list<std::unique_ptr<View>> views;
#if HAVE_XWAYLAND
    std::list<std::unique_ptr<XwaylandORSurface>> xwayland_or_surfaces;
#endif
    LayerArray layers;

    /// Common mapping procedure for views regardless of their underlying shell.
    void map_view(Server&, View&);
    /// Common unmapping procedure for views regardless of their underlying shell.
    void unmap_view(Server&, View&);
    /// Puts the \a view on top.
    void move_view_to_front(View&);

    Workspace& get_views_workspace(Server&, View&);

    /**
     * \brief Returns the xdg / xwayland / layer_shell surface leaf of the first
     * view / layer / xwayland override redirect surface under the cursor.
     *
     * \a lx and \a ly are root (output layout) coordinates. That is, coordinates relative to the imaginary plane of all surfaces.
     *
     * \param[out] surface The \c wlr_surface found under the cursor.
     * \param[out] sx The x coordinate of the found surface in root coordinates.
     * \param[out] sy The y coordinate of the found surface in root coordinates.
     */
    OptionalRef<View> get_surface_under_cursor(Server&, double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy);
};

#endif // CARDBOARD_VIEW_MANAGER_H_INCLUDED
