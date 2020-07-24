extern "C" {
#include <wlr/util/log.h>
}

#include <algorithm>
#include <cassert>
#include <unordered_set>

#include "OptionalRef.h"
#include "Output.h"
#include "View.h"
#include "ViewOperations.h"
#include "Workspace.h"

std::unordered_set<NotNullPointer<View>> Workspace::Column::get_mapped_and_normal_set()
{
    std::unordered_set<NotNullPointer<View>> set;
    for (auto& tile : tiles) {
        if (tile.view->is_mapped_and_normal()) {
            set.insert(tile.view);
        }
    }

    return set;
}

std::list<Workspace::Column>::iterator Workspace::find_column(View* view)
{
    return std::find_if(columns.begin(), columns.end(), [view](const auto& column) {
        return std::find_if(column.tiles.begin(), column.tiles.end(), [view](const auto& tile) {
                   return tile.view == view;
               })
            != column.tiles.end();
    });
}

std::list<NotNullPointer<View>>::iterator Workspace::find_floating(View* view)
{
    // view can be null. if that's the case, this function will return floating_views.end()

    // this if is not needed but it might be a slight optimisation
    if (view == nullptr) {
        return floating_views.end();
    }

    return std::find_if(floating_views.begin(), floating_views.end(), [view](const auto& v) {
        return v == view;
    });
}

void Workspace::add_view(OutputManager& output_manager, View& view, View* next_to, bool floating, bool transferring)
{
    // if next_to is null, view will be added at the end of the list
    if (floating) {
        auto it = find_floating(next_to);

        floating_views.insert(
            it == floating_views.end() ? it : ++it,
            &view);
    } else {
        auto it = find_column(next_to);
        if (it != columns.end()) {
            std::advance(it, 1);
        }
        auto new_it = columns.insert(it, Column {});
        new_it->tiles.push_back({ &view, &*new_it });
    }

    if (!transferring) {
        view.workspace_id = index;

        if (output) {
            view.set_activated(true);
        }
        view.change_output(NullRef<Output>, output);
    }

    arrange_workspace(output_manager);
}

void Workspace::remove_view(OutputManager& output_manager, View& view, bool transferring)
{
    if (!transferring) {
        if (fullscreen_view && &fullscreen_view.unwrap() == &view) {
            fullscreen_view = NullRef<View>;
        }
        view.set_activated(false);
        view.change_output(output, NullRef<Output>);
    }
    auto column_it = find_column(&view);
    if (column_it != columns.end()) {
        column_it->tiles.remove_if([&view](auto& other) { return other.view == &view; });
        // destroy column if no tiles left
        if (column_it->tiles.empty()) {
            columns.erase(column_it);
        }
    }
    floating_views.remove(&view);

    arrange_workspace(output_manager);
}

void Workspace::insert_into_column(OutputManager& output_manager, View& view, Column& column)
{
    int max_width = 0;
    for (auto& tile : column.tiles) {
        if (tile.view->is_mapped_and_normal()) {
            max_width = std::max(max_width, tile.view->geometry.width);
        }
    }
    remove_view(output_manager, view, true);
    column.tiles.push_back({ &view, &column });

    // Match view's width with the rest of the column.
    // You might consider this a terrible hack. It makes arrange_workspace "think" that the view has been resized.
    // The correct width is going to be set in arrange_workspace anyway.
    view.geometry.width = max_width;

    arrange_workspace(output_manager);
}

void Workspace::pop_from_column(OutputManager& output_manager, Column& column)
{
    if (column.tiles.size() < 2) {
        return;
    }

    auto& to_pop = *column.tiles.back().view;
    auto& next_to = *column.tiles.front().view;
    remove_view(output_manager, to_pop);
    add_view(output_manager, to_pop, &next_to, false, true);

    arrange_workspace(output_manager);
}

void Workspace::arrange_workspace(OutputManager& output_manager, bool animate)
{
    if (!output) {
        return;
    }

    int acc_width = 0;
    const struct wlr_box* output_box = output_manager.get_output_box(output.unwrap());
    const struct wlr_box& usable_area = output.unwrap().usable_area;

    fullscreen_view.and_then([output_box](auto& view) {
        view.move(output_box->x - view.geometry.x, output_box->y - view.geometry.y);
        view.resize(output_box->width, output_box->height);
    });

    // arrange tiles
    for (auto& column : columns) {
        // ignore columns that are not ready for tiling
        bool should_skip = false;
        float scale_sum = 0; // sum of all weights for height calculation
        for (auto& tile : column.tiles) {
            if (!tile.view->is_mapped_and_normal()) {
                should_skip = true;
            } else {
                scale_sum += tile.vertical_scale;
            }
        }
        if (should_skip) {
            continue;
        }

        int current_y = output_box->y + usable_area.y + server->config.gap;
        int max_width = 0;
        for (auto& tile : column.tiles) {
            auto& view = *tile.view;

            max_width = std::max(max_width, tile.view->geometry.width);

            view.target_x = output_box->x + acc_width - view.geometry.x - scroll_x;
            view.target_y = current_y - view.geometry.y;

            if (animate) {
                server->view_animation->enqueue_task({
                                                             tile.view,
                                                             tile.view->target_x,
                                                             tile.view->target_y
                                                     });
            } else {
                view.x = view.target_x;
                view.y = view.target_y;
            }

            int height = static_cast<int>(
                    static_cast<float>(
                        usable_area.height - (column.tiles.size() + 1) * server->config.gap
                    ) * (tile.vertical_scale / scale_sum));
            view.resize(view.geometry.width, height);

            current_y += height + server->config.gap;
        }

        acc_width += max_width + server->config.gap;
    }
}

void Workspace::fit_view_on_screen(OutputManager& output_manager, View& view, bool condense)
{
    // don't do anything if we have a fullscreened view or the previously
    // fullscreened view hasn't been restored
    if (fullscreen_view || view.expansion_state != View::ExpansionState::NORMAL) {
        return;
    }

    auto column_it = find_column(&view);
    if (column_it == columns.end()) {
        return;
    }

    if (!output) {
        return;
    }

    const auto& output = this->output.unwrap();
    const struct wlr_box* output_box = output_manager.get_output_box(output);

    const auto usable_area = output.usable_area;
    int wx = get_view_wx(view);
    int vx = view.x + view.geometry.x;

    bool overflowing = vx < 0 || view.target_x + view.geometry.x + view.geometry.width > usable_area.x + usable_area.width;
    if (condense && &*column_it == &*columns.begin()) {
        // align first window to the display's left edge
        scroll_x = -usable_area.x + server->config.gap / 2;
    } else if (condense && &*column_it == &*columns.rbegin()) { // epic identity checking
        // align last window to the display's right edge
        scroll_x = wx + view.geometry.width - (usable_area.x + usable_area.width) - server->config.gap / 2;
    } else if (overflowing && vx < output_box->x + usable_area.x) {
        scroll_x = wx - usable_area.x - server->config.gap / 2;
    } else if (overflowing && vx + view.geometry.width >= output_box->x + usable_area.x + usable_area.width) {
        scroll_x = wx + view.geometry.width - (usable_area.x + usable_area.width) + server->config.gap / 2;
    }

    arrange_workspace(output_manager);
}

OptionalRef<View> Workspace::find_dominant_view(OutputManager& output_manager, Seat& seat, OptionalRef<View> focused_view)
{
    if (!output) {
        return NullRef<View>;
    }

    Workspace::Column* most_visible = nullptr;
    double maximum_visibility = 0;
    double focused_view_visibility = 0;
    const auto usable_area = output_manager.get_output_real_usable_area(output.unwrap());

    // we will find the most visible column, based on its width and position,
    // and select the most recently focused tile
    for (auto& column : columns) {
        // Find first mapped view of the column.
        // We need it only for its width.
        View* view = nullptr;
        for (auto& tile : column.tiles) {
            view = tile.view;
            if (view->mapped) {
                break;
            }
        }
        if (!view) {
            continue;
        }

        // let's consider that our column has only one view
        const struct wlr_box view_box = {
            .x = view->x + view->geometry.x,
            .y = usable_area.x + view->geometry.y,
            .width = view->geometry.width,
            .height = usable_area.height,
        };
        struct wlr_box intersection;
        bool ok = wlr_box_intersection(&intersection, &usable_area, &view_box);
        if (!ok) {
            // no intersection, view isn't on the screen at all
            continue;
        }
        double visibility = static_cast<double>(intersection.width * intersection.height) / (view_box.width * view_box.height);
        if (visibility > maximum_visibility) {
            maximum_visibility = visibility;
            most_visible = &column;
        }

        bool focused_view_in_this_column = false;
        for (auto& tile : column.tiles) {
            if (tile.view == focused_view.raw_pointer()) {
                focused_view_in_this_column = true;
            }
        }
        if (focused_view_in_this_column) {
            focused_view_visibility = visibility;
        }
    }

    if (!focused_view || (focused_view && maximum_visibility - focused_view_visibility > 0.01)) {
        auto view_set = most_visible->get_mapped_and_normal_set();
        for (auto view_ptr : seat.focus_stack) {
            if (view_set.contains(view_ptr)) {
                return OptionalRef(view_ptr);
            }
        }
    }

    return focused_view;
}

int Workspace::get_view_wx(View& view)
{
    int acc_wx = 0;

    for (auto& column : columns) {
        View* reference_view = nullptr;
        for (auto& tile : column.tiles) {
            if (tile.view == &view) {
                return acc_wx;
            }

            if (tile.view->is_mapped_and_normal()) {
                reference_view = tile.view;
            }
        }
        acc_wx += reference_view->geometry.width + server->config.gap;
    }

    return acc_wx;
}

void Workspace::set_fullscreen_view(OutputManager& output_manager, OptionalRef<View> view)
{
    fullscreen_view.and_then([](auto& fview) {
        fview.set_fullscreen(false);
        fview.expansion_state = View::ExpansionState::RECOVERING;
        fview.move(fview.saved_state->x, fview.saved_state->y);
        fview.resize(fview.saved_state->width, fview.saved_state->height);
        fview.saved_state = std::nullopt;
    });
    view.and_then([](auto& v) {
        v.save_state({
            .x = v.x,
            .y = v.y,
            .width = v.geometry.width,
            .height = v.geometry.height,
        });
        v.expansion_state = View::ExpansionState::FULLSCREEN;
        v.set_fullscreen(true);
    });
    fullscreen_view = view;

    arrange_workspace(output_manager);
}

bool Workspace::is_view_floating(View& view)
{
    return std::find(floating_views.begin(), floating_views.end(), &view) != floating_views.end();
}

void Workspace::activate(Output& new_output)
{
    for (const auto& column : columns) {
        for (const auto& tile : column.tiles) {
            tile.view->change_output(output, new_output);
            tile.view->set_activated(true);
        }
    }

    output = OptionalRef<Output>(new_output);
}

void Workspace::deactivate()
{
    if (!output) {
        return;
    }
    for (const auto& column : columns) {
        for (const auto& tile : column.tiles) {
            tile.view->change_output(output.unwrap(), NullRef<Output>);
            tile.view->set_activated(false);
        }
    }
    output = NullRef<Output>;
}