#include <wlr_cpp/util/edges.h>
#include <wlr_cpp/util/log.h>

#include <algorithm>
#include <cassert>

#include "View.h"
#include "Workspace.h"

Workspace::Workspace(IndexType index)
    : index(index)
    , scroll_x(0)
{
}

void Workspace::set_output_layout(struct wlr_output_layout* ol)
{
    output_layout = ol;
}

std::list<Workspace::Tile>::iterator Workspace::find_tile(View* view)
{
    return std::find_if(tiles.begin(), tiles.end(), [view](const auto& t) {
        return t.view == view;
    });
}

void Workspace::add_view(View* view, View* next_to)
{
    auto it = find_tile(next_to);
    if (it != tiles.end()) {
        std::advance(it, 1);
    }
    auto tile = tiles.insert(it, { view });

    assert(output.has_value());

    int ow, oh;
    wlr_output_effective_resolution(*output, &ow, &oh);

    tile->view->workspace_id = index;
    tile->view->resize(tile->view->geometry.width, oh);
    wlr_log(WLR_DEBUG, "resizing to %d - %d", oh, view->geometry.y);

    arrange_tiles();
}

void Workspace::remove_view(View* view)
{
    tiles.remove_if([view](auto& other) { return other.view == view; });

    arrange_tiles();
}

void Workspace::arrange_tiles()
{
    int acc_width = 0;
    for (auto& tile : tiles) {
        tile.view->x = acc_width - tile.view->geometry.x - scroll_x;
        tile.view->y = -tile.view->geometry.y;

        acc_width += tile.view->geometry.width;
    }
}

bool Workspace::is_spanning()
{
    if (tiles.empty()) {
        return false;
    }

    assert(output.has_value());

    const auto* output_box = wlr_output_layout_get_box(output_layout, *output);
    if (output_box == nullptr) {
        return false;
    }

    int acc_width = 0;
    for (const auto& tile : tiles) {
        acc_width += tile.view->geometry.width;
        if (acc_width >= output_box->width) {
            return true;
        }
    }

    return false;
}

void Workspace::fit_view_on_screen(View* view)
{
    if (view == nullptr) {
        return;
    }

    auto it = find_tile(view);
    if (it == tiles.end()) {
        return;
    }

    assert(output.has_value());

    const auto* output_box = wlr_output_layout_get_box(output_layout, *output);
    if (output_box == nullptr) {
        return;
    }

    int vx = view->x + view->geometry.x;

    bool spanning = is_spanning();
    if (spanning && view == tiles.begin()->view) {
        // align first window to the display's left edge
        scroll_x += vx - output_box->x;
    } else if (spanning && view == tiles.rbegin()->view) {
        // align last window to the display's right edge
        scroll_x += vx + view->geometry.width - output_box->x - output_box->width;
    } else if (vx < output_box->x) {
        scroll_x += vx - output_box->x;
    } else if (vx + view->geometry.width >= output_box->x + output_box->width) {
        scroll_x += vx + view->geometry.width - output_box->x - output_box->width;
    }

    arrange_tiles();
}

int Workspace::get_view_wx(View* view)
{
    int acc_wx = 0;

    for (auto& tile : tiles) {
        if (tile.view == view) {
            break;
        }
        acc_wx += tile.view->geometry.width;
    }

    return acc_wx;
}

void Workspace::deactivate()
{
    output = std::nullopt;
}
