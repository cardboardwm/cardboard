#ifndef __CARDBOARD_TILING_H_
#define __CARDBOARD_TILING_H_

extern "C" {
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
}

#include <algorithm>
#include <list>
#include <optional>

#include "OptionalRef.h"

class View;
struct Output;
struct Server;

/**
 * \brief A Workspace is a group of tiled windows.
 *
 * Workspaces are assigned to at most one output.
 *
 * The workspace can be imagined as a plane with vertically maxed
 * views placed side-by-side. The plane has its origin in the origin of its first view.
 * Parts of this plane are visible on the output, regarded as the viewport.
 *
 * This viewport can be moved on the horizontal axis of the plane, like a sliding window.
 * As a result, only a segment of these horizontally aligned views is shown on the screen. The
 * scrolling of the viewport is controlled by the #scroll_x variable.
 */
struct Workspace {
    using IndexType = ssize_t;
    struct Tile {
        View* view;
    };

    std::list<Tile> tiles;
    std::list<View*> floating_views;
    struct wlr_output_layout* output_layout;

    /**
     * \brief The output assigned to this workspace (or the output to which this workspace is assigned).
     *
     * It's equal to \c std::nullopt if this workspace isn't assigned.
     */
    OptionalRef<Output> output;
    /// The currently full screened view, if any.
    OptionalRef<View> fullscreen_view;
    std::optional<std::pair<int, int>> fullscreen_original_size;

    IndexType index;

    /**
     * \brief The offset of the viewport.
     */
    int scroll_x = 0;

    Workspace(IndexType index);

    /**
     * \brief Sets the wlr_output_layout pointer to \a ol.
     */
    void set_output_layout(struct wlr_output_layout* ol);

    /**
     * \brief Returns an iterator to the tile containing \a view.
     */
    std::list<Tile>::iterator find_tile(View* view);

    /**
     * \brief Returns an iterator to the a floating view.
     */
    std::list<View*>::iterator find_floating(View* view);

    /**
    * \brief Adds the \a view to the right of the \a next_to view and tiles it accordingly.
    */
    void add_view(View* view, View* next_to, bool floating = false);

    /**
    * \brief Removes \a view from the workspace and tiles the others accordingly.
    */
    void remove_view(View* view);

    /**
    * \brief Puts the windows in tiled position.
    */
    void arrange_tiles();

    /**
    * \brief Returns \c true if the views of the workspace overflow the output \a output.
    */
    bool is_spanning();

    /**
    * \brief Scrolls the viewport of the workspace just enough to make the
    * entirety of \a view visible, i.e. there are no off-screen parts of it.
    */
    void fit_view_on_screen(View* view);

    /**
    * \brief Returns the x coordinate of \a view in workspace coordinates.
    * The origin of the workspace plane is the top-left corner of the first window,
    * be it off-screen or not.
    */
    int get_view_wx(View*);

    /// Sets \a view as the currently fullscreen view. If null, the fullscreen view will be cleared, if any.
    void set_fullscreen_view(View* view);

    /// Returns true if the view is floating in this Workspace.
    bool is_view_floating(View* view);

    /**
     * \brief Assigns the workspace to an \a output.
     */
    void activate(Output& output);

    /**
     * \brief Marks the workspace as inactive: it is not assigned to any output.
     */
    void deactivate();
};

#endif //  __CARDBOARD_TILING_H_
