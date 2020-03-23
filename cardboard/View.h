#ifndef __CARDBOARD_VIEW_H_
#define __CARDBOARD_VIEW_H_

#include <wayland-server.h>
#include <wlr_cpp/types/wlr_output.h>
#include <wlr_cpp/types/wlr_output_layout.h>
#include <wlr_cpp/types/wlr_xdg_shell.h>

#include <list>

#include "Workspace.h"

struct Server;

/**
 * \brief Represents a normal window on the screen.
 *
 * Each View has its base in a shell (xdg-shell, layer-shell, XWayland). Currently, only
 * xdg-shell is supported. The View represents an xdg-shell toplevel surface.
 *
 * These windows may be associated with a Workspace, if its tiled. You can read about the tiled behaviour
 * in the documentation of the Workspace class.
 *
 * Views can also be mapped or unmapped, shown or hidden. Mapped and unmapped are states defined by the shell.
 * Usually, a mapped View is a window shown on the screen ready for user action, while an unmapped View is
 * deactivated and hidden (think minimized). On the other hand, shown and hidden are specific to the compositor.
 * A shown View is currently displayed on an output (screen), in an active workspace
 * (a workspace must be shown on an output to be active). It is visible. A hidden View is a View that is mapped,
 * but not visible on the screen. This happens when the View is mapped in a deactivated Workspace.
 *
 * So a shown View is always mapped, but a mapped View is not always shown. An unmapped View is always hidden,
 * but a hidden View is not always unmapped.
 *
 * There is only a flag for the mapped state (View::mapped), which is set by the unmap event handler of the
 * corresponding shell.
 *
 * \sa <a href="https://drewdevault.com/2018/07/29/Wayland-shells.html">Writing a Wayland compositor with wlroots: shells</a>
 * by Drew DeVault
 */
struct View {
    struct wlr_xdg_surface* xdg_surface;
    /**
     * \brief The size and offset of the usable working area.
     *
     * Unlike X11, Wayland only has the concept of a \c wl_surface, which is a rectangular region
     * that displays pixels and receives input. Layers give a meaning to these surfaces.
     *
     * These surfaces also don't know anything about the screen they are drawn on, not even their position
     * on the screen. As these surfaces are just rectangles with pixels, they are also used to contain
     * shadows and decorations that the user can't interact with, near the border of the surface. This is why
     * inside this rectangle there is an inner rectangle, positioned at an <tt>(x, y)</tt> offset from the
     * top-left corner of the surface with a \c width and a \c height that is the usable region of the surface.
     * This is what the user perceives as a window.
     *
     * This inner rectangle is called a \a geometry.
     *
     * \attention This property should not be modified directy, but by the resize() function.
     * Geometry modifications must be communicated to the client and the client must acknowledge
     * how much it will change. The property will change according to the desire of the client.
     * The change happens in xdg_surface_commit_handler().
     */
    struct wlr_box geometry;

    /// The id of the workspace this View is assigned to. Set to -1 if none.
    Workspace::IndexType workspace_id;

    int x, y; ///< Coordinates of the surface, relative to the output layout (root coordinates).
    bool mapped;

    /**
     * \brief Constructs the View struct from an \c xdg_surface.
     *
     * The struct is only initialized with data, it is not registered to the compositor by this constructor.
     */
    View(struct wlr_xdg_surface* xdg_surface);
    /**
     * \brief Gets the child sub-surface of this view's toplevel xdg surface sitting under a point, if exists.
     *
     * \a lx and \a ly are the given point, in output layout relative coordinates.
     *
     * \param[out] surface the surface under the cursor, if found.
     * \param[out] sx the x coordinate of the given point, relative to the surface.
     * \param[out] sy the y coordinate of the given point, relative to the surface
     *
     * \returns \c true if there is a surface underneath the point, \c false if there isn't.
     */
    //
    // sx and sy and the given output layout relative coordinates (lx and ly), relative to that surface
    bool get_surface_under_coords(double lx, double ly, struct wlr_surface*& surface, double& sx, double& sy);

    /// Requests the resize to the client. Do not assume that the client is resized afterwards.
    void resize(int width, int height);
};

void xdg_surface_map_handler(struct wl_listener* listener, void* data);
void xdg_surface_unmap_handler(struct wl_listener* listener, void* data);
void xdg_surface_destroy_handler(struct wl_listener* listener, void* data);
void xdg_surface_commit_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_move_handler(struct wl_listener* listener, void* data);
void xdg_toplevel_request_resize_handler(struct wl_listener* listener, void* data);

/// Registers a view to the server and attaches the event handlers.
void create_view(Server* server, View&& view);

#endif // __CARDBOARD_VIEW_H_
