#include "Tiling.h"

#include <wlr_cpp/util/edges.h>
#include <wlr_cpp/util/log.h>

#include <algorithm>

// TODO: remove this
const int TILE_WIDTH = 300;

void TilingSequence::add_view(View *view, View* next_to)
{
    auto it = std::find_if(tiles.begin(), tiles.end(), [next_to](auto& comp) {
        return comp.view == next_to;
    });
    if (it != tiles.end()) {
        std::advance(it, 1);
    }
    auto tile = tiles.insert(it, { view });

    auto output = wlr_output_layout_output_at(output_layout, (double)view->x + view->geometry.width / 2.,
                                              (double)view->y + view->geometry.height / 2.);
    int ow, oh;
    wlr_output_effective_resolution(output, &ow, &oh);

    tile->view->resize(TILE_WIDTH, oh);
    wlr_log(WLR_DEBUG, "resizing to %d - %d", oh, view->geometry.y);

    arrange_tiles();
}

void TilingSequence::remove_view(View *view)
{
    if (auto it = std::find_if(tiles.begin(), tiles.end(), [view](const auto& t) {
            return t.view == view;
        });
        it != tiles.end()) {

        tiles.erase(it);
    }
    arrange_tiles();
}

void TilingSequence::arrange_tiles()
{
    int acc_width = 0;
    for (auto& tile : tiles) {
        tile.view->x = acc_width - tile.view->geometry.x;
        tile.view->y = -tile.view->geometry.y;

        acc_width += tile.view->geometry.width;
    }
}
