#ifndef __CARDBOARD_TILING_H_
#define __CARDBOARD_TILING_H_

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

    void add_view(View* view, View* next_to);
    void remove_view(View* view);
    void arrange_tiles();
};

#endif //  __CARDBOARD_TILING_H_
