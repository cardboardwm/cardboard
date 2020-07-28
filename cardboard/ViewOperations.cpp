#include "ViewOperations.h"

/// Moves the view from a workspace to another. Handles special cases like fullscreen views, floating views etc.
void change_view_workspace(Server& server, View& view, Workspace& new_workspace)
{
    Workspace& workspace = server.output_manager->get_view_workspace(view);

    if (&workspace == &new_workspace) {
        return;
    }

    if (view.expansion_state == View::ExpansionState::FULLSCREEN) {
        if (new_workspace.fullscreen_view) {
            new_workspace.set_fullscreen_view(*(server.output_manager), NullRef<View>);
        }
        new_workspace.fullscreen_view = OptionalRef(view);
        new_workspace.arrange_workspace(*(server.output_manager));
    }

    bool floating = workspace.is_view_floating(view);
    if (floating && new_workspace.output.has_value() && view.expansion_state != View::ExpansionState::FULLSCREEN) {
        auto output_area = server.output_manager->get_output_real_usable_area(new_workspace.output.unwrap());

        if (view.x < output_area.x || view.x >= output_area.x + output_area.width || view.y < output_area.y || view.y >= output_area.y + output_area.height) {
            view.move(
                output_area.x + output_area.width / 2,
                output_area.y + output_area.height / 2);
        }
    }

    workspace.remove_view(*(server.output_manager), view);
    new_workspace.add_view(*(server.output_manager), view, nullptr, floating);

    if (auto last_focused_view = std::find_if(server.seat.focus_stack.begin(), server.seat.focus_stack.end(), [&workspace, &view](View* v) {
            return v->workspace_id == workspace.index && v != &view;
        });
        last_focused_view != server.seat.focus_stack.end()) {
        server.seat.focus_view(server, OptionalRef<View>(*last_focused_view));
    } else {
        server.seat.focus_view(server, NullRef<View>);
    };
    cursor_rebase(server, server.seat, server.seat.cursor);
}

/// If a floating view changed the output it appears on (for example by dragging), move it to that output's workspace.
static void update_view_workspace(Server& server, View& view)
{
    if (server.output_manager->workspaces[view.workspace_id].find_floating(&view) != server.output_manager->workspaces[view.workspace_id].floating_views.end()) {
        OptionalRef<Output> current_output = server.output_manager->get_output_at(view.x, view.y);

        if (current_output && current_output != server.output_manager->workspaces[view.workspace_id].output) {
            auto workspace = std::find_if(server.output_manager->workspaces.begin(), server.output_manager->workspaces.end(), [current_output](auto& w) {
                return w.output == current_output;
            });

            change_view_workspace(server, view, *workspace);
        }
    }
}

/// Does the appropriate movement for tiled and floating views. When moved, tiled views scroll the workspace, and floating views need to be updated when changing outputs.
void reconfigure_view_position(Server& server, View& view, int x, int y, bool animate)
{
    if (auto& workspace = server.output_manager->workspaces[view.workspace_id]; workspace.find_column(&view) != workspace.columns.end()) {
        int dx = view.x - x;

        scroll_workspace(*(server.output_manager), workspace, RelativeScroll { dx }, animate);
    } else {
        view.move(x, y);
        update_view_workspace(server, view);
    }
}

/// Resizes \a view to the given size. Tiled views have their heights determined by the tiling algorithm, therefore they don't change height.
void reconfigure_view_size(Server& server, View& view, int width, int height)
{
    auto& workspace = server.output_manager->get_view_workspace(view);

    if (auto column_it = workspace.find_column(&view); column_it != workspace.columns.end()) {
        height = view.geometry.height;
        for (auto& tile : column_it->mapped_and_normal_tiles()) {
            tile.view->resize(width, height);
        }
    } else {
        view.resize(width, height);
    }
}

/// Sets the workspace scroll to an absolute value.
void scroll_workspace(OutputManager& output_manager, Workspace& workspace, AbsoluteScroll scroll, bool animate)
{
    workspace.scroll_x = scroll.get();
    workspace.arrange_workspace(output_manager, animate);
}

/// Scrolls the workspace by a delta value.
void scroll_workspace(OutputManager& output_manager, Workspace& workspace, RelativeScroll scroll, bool animate)
{
    workspace.scroll_x += scroll.get();
    workspace.arrange_workspace(output_manager, animate);
}
