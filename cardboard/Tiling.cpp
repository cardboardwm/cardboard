#include "Tiling.h"

#include <wlr_cpp/util/edges.h>
#include <wlr_cpp/util/log.h>

#include <algorithm>

void TilingSequence::add_view(View* view, View* next_to)
{
    auto it = find_tile(next_to);
    if (it != tiles.end()) {
        std::advance(it, 1);
    }
    auto tile = tiles.insert(it, { view });

    auto output = view->get_closest_output(output_layout);

    if (output != nullptr) {
        int ow, oh;
        wlr_output_effective_resolution(output, &ow, &oh);

        tile->view->resize(tile->view->geometry.width, oh);
        wlr_log(WLR_DEBUG, "resizing to %d - %d", oh, view->geometry.y);
    } else {
        wlr_log(WLR_ERROR, "cannot resize view, it's off-screen!");
    }

    arrange_tiles();
}

void TilingSequence::remove_view(View* view)
{
    if (auto it = find_tile(view); it != tiles.end()) {
        tiles.erase(it);
    }
    arrange_tiles();
}

void TilingSequence::arrange_tiles()
{
    int acc_width = 0;
    for (auto& tile : tiles) {
        tile.view->x = acc_width - tile.view->geometry.x - scroll_x;
        tile.view->y = -tile.view->geometry.y;

        acc_width += tile.view->geometry.width;
    }
}

bool TilingSequence::is_spanning(wlr_output* output)
{
    if (tiles.empty()) {
        return false;
    }

    const auto* output_box = wlr_output_layout_get_box(output_layout, output);
    int acc_width = 0;
    for (const auto& tile : tiles) {
        acc_width += tile.view->geometry.width;
        if (acc_width >= output_box->width) {
            return true;
        }
    }

    return false;
}

void TilingSequence::fit_view_on_screen(View* view)
{
    if (view == nullptr) {
        return;
    }

    auto it = find_tile(view);
    if (it == tiles.end()) {
        return;
    }

    auto output = view->get_closest_output(output_layout);
    if (output == nullptr) {
        return;
    }

    const auto* output_box = wlr_output_layout_get_box(output_layout, output);
    int vx = view->x + view->geometry.x;

    bool spanning = is_spanning(output);
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

int TilingSequence::get_view_tx(View* view)
{
    int acc_tx = 0;
    bool found = false;
    for (auto& tile : tiles) {
        if (tile.view == view) {
            found = true;
            break;
        }
        acc_tx += tile.view->geometry.width;
    }

    return acc_tx;
}
