extern "C" {
#include <wlr/util/log.h>
}

#include <algorithm>
#include <cassert>

#include "OptionalRef.h"
#include "Output.h"
#include "View.h"
#include "ViewManager.h"
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

std::list<View*>::iterator Workspace::find_floating(View* view)
{
    return std::find_if(floating_views.begin(), floating_views.end(), [view](const auto& v) {
        return v == view;
    });
}

void Workspace::add_view(View* view, View* next_to, bool floating, bool transferring)
{
    // if next_to is null, view will be added at the end of the list
    if (floating) {
        auto it = find_floating(next_to);

        floating_views.insert(
            it == floating_views.end() ? it : ++it,
            view);
    } else {
        auto it = find_tile(next_to);
        if (it != tiles.end()) {
            std::advance(it, 1);
        }
        tiles.insert(it, { view });
    }

    if (!transferring) {
        view->workspace_id = index;

        if (output) {
            view->set_activated(true);
        }
        view->change_output(nullptr, output);
    }

    arrange_workspace();
}

void Workspace::remove_view(View* view, bool transferring)
{
    if (!transferring) {
        if (fullscreen_view && &fullscreen_view.unwrap() == view) {
            fullscreen_view = NullRef<View>;
        }
        view->set_activated(false);
        view->change_output(output, nullptr);
    }
    tiles.remove_if([view](auto& other) { return other.view == view; });
    floating_views.remove(view);

    arrange_workspace();
}

void Workspace::arrange_workspace()
{
    if (!output) {
        return;
    }

    int acc_width = 0;
    const auto* output_box = wlr_output_layout_get_box(output_layout, output.unwrap().wlr_output);
    const struct wlr_box& usable_area = output.unwrap().usable_area;

    fullscreen_view.and_then([output_box](auto& view) {
        view.move(output_box->x - view.geometry.x, output_box->y - view.geometry.y);
        view.resize(output_box->width, output_box->height);
    });

    // arrange tiles
    for (auto& tile : tiles) {
        if (!tile.view->mapped || tile.view->expansion_state == View::ExpansionState::FULLSCREEN || tile.view->expansion_state == View::ExpansionState::RECOVERING) {
            continue;
        }
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
    // don't do anything if we have a fullscreened view or the previously
    // fullscreened view hasn't been restored
    if (view->expansion_state != View::ExpansionState::NORMAL) {
        return;
    }

    if (view == nullptr) {
        return;
    }

    if (find_tile(view) == tiles.end()) {
        return;
    }

    if (!output) {
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

    arrange_workspace();
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

void Workspace::set_fullscreen_view(View* view)
{
    fullscreen_view.and_then([](auto& fview) {
        fview.set_fullscreen(false);
        fview.expansion_state = View::ExpansionState::RECOVERING;
        fview.move(fview.saved_state->x, fview.saved_state->y);
        fview.resize(fview.saved_state->width, fview.saved_state->height);
        fview.saved_state = std::nullopt;
    });
    if (view != nullptr) {
        view->save_state({
            .x = view->x,
            .y = view->y,
            .width = view->geometry.width,
            .height = view->geometry.height,
        });
        view->expansion_state = View::ExpansionState::FULLSCREEN;
        view->set_fullscreen(true);
    }
    fullscreen_view = OptionalRef(view);

    arrange_workspace();
}

bool Workspace::is_view_floating(View* view)
{
    return std::find(floating_views.begin(), floating_views.end(), view) != floating_views.end();
}

void Workspace::activate(Output& new_output)
{
    for (const auto& tile : tiles) {
        tile.view->change_output(output, new_output);
        tile.view->set_activated(true);
    }

    output = OptionalRef<Output>(new_output);
}

void Workspace::deactivate()
{
    if (!output) {
        return;
    }
    for (const auto& tile : tiles) {
        tile.view->change_output(output.unwrap(), NullRef<Output>);
        tile.view->set_activated(false);
    }
    output = NullRef<Output>;
}
