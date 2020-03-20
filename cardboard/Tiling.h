#ifndef __CARDBOARD_TILING_H_
#define __CARDBOARD_TILING_H_

#include <algorithm>
#include <list>

#include <wlr_cpp/types/wlr_output_layout.h>

#include "View.h"

struct TilingSequence {
    struct Tile {
        View* view;
    };

    std::list<Tile> tiles;
    struct wlr_output_layout* output_layout;

    int scroll_x = 0;

    void set_output_layout(struct wlr_output_layout* ol)
    {
        output_layout = ol;
    }

    auto find_tile(View* view)
    {
        return std::find_if(tiles.begin(), tiles.end(), [view](const auto& t) {
            return t.view == view;
        });
    }

    void add_view(View* view, View* next_to);
    void remove_view(View* view);
    void arrange_tiles();

    // returns true if the sum of the tiles' widths are larger than the output's width
    bool is_spanning(wlr_output* output);

    void fit_view_on_screen(View* view);
    // tx is the x coordinate relative to the tiling sequence.
    // The first view is always tx = 0
    int get_view_tx(View*);
};

#endif //  __CARDBOARD_TILING_H_
