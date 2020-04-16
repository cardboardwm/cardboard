extern "C" {
#include <wlr/util/log.h>
}

#include <algorithm>
#include <cassert>

#include "Output.h"
#include "OptionalRef.h"
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
    // if next_to is null, view will be added at the end of the list
    auto it = find_tile(next_to);
    if (it != tiles.end()) {
        std::advance(it, 1);
    }
    auto tile = tiles.insert(it, { view });

    assert(output.has_value());

    tile->view->workspace_id = index;

    arrange_tiles();
}

void Workspace::remove_view(View* view)
{
    tiles.remove_if([view](auto& other) { return other.view == view; });

    arrange_tiles();
}

void Workspace::arrange_tiles()
{
    if (!output) {
        return;
    }

    int acc_width = 0;
    const auto* output_box = wlr_output_layout_get_box(output_layout, output.unwrap().wlr_output);
    const struct wlr_box& usable_area = output.unwrap().usable_area;

    for (auto& tile : tiles) {
        tile.view->x = output_box->x + acc_width - tile.view->geometry.x - scroll_x;
        tile.view->y = output_box->y + usable_area.y - tile.view->geometry.y;
        tile.view->resize(tile.view->geometry.width, usable_area.height);

        acc_width += tile.view->geometry.width;
    }
}

bool Workspace::is_spanning()
{
    if (tiles.empty()) {
        return false;
    }

    const auto& output = this->output.unwrap();
    const auto& usable_area = output.usable_area;

    for (const auto& tile : tiles) {
        double ox = tile.view->x + tile.view->geometry.x, oy = tile.view->y + tile.view->geometry.y;
        wlr_output_layout_output_coords(output_layout, output.wlr_output, &ox, &oy);

        if (ox < usable_area.x || ox + tile.view->geometry.width > usable_area.x + usable_area.width) {
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

    if (find_tile(view) == tiles.end()) {
        return;
    }

    const auto& output = this->output.unwrap();
    const auto* output_box = wlr_output_layout_get_box(output_layout, output.wlr_output);
    if (output_box == nullptr) {
        return;
    }
    const auto usable_area = output.usable_area;
    int wx = get_view_wx(view);
    int vx = view->x + view->geometry.x;

    bool spanning = is_spanning();
    if (view == tiles.begin()->view) {
        // align first window to the display's left edge
        scroll_x = -usable_area.x;
    } else if (spanning && view == tiles.rbegin()->view) {
        // align last window to the display's right edge
        scroll_x = wx + view->geometry.width - (usable_area.x + usable_area.width);
    } else if (vx < output_box->x + usable_area.x) {
        scroll_x = wx - usable_area.x;
    } else if (vx + view->geometry.width >= output_box->x + usable_area.x + usable_area.width) {
        scroll_x = wx + view->geometry.width - (usable_area.x + usable_area.width);
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

void Workspace::activate(Output& new_output)
{
    for (const auto& tile : tiles) {
        tile.view->change_output(output, new_output);
    }

    output = OptionalRef<Output>(new_output);
}

void Workspace::deactivate()
{
    for (const auto& tile : tiles) {
        tile.view->change_output(output.unwrap(), NullRef<Output>);
    }
    output = NullRef<Output>;
}
