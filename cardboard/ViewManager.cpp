#include "ViewManager.h"

void change_view_workspace(NotNullPointer<Server> server, NotNullPointer<View> view, NotNullPointer<Workspace> new_workspace)
{
    Workspace& workspace = server->get_views_workspace(view);

    if (&workspace == new_workspace.get()) {
        return;
    }

    if (view->expansion_state == View::ExpansionState::FULLSCREEN) {
        if (new_workspace->fullscreen_view) {
            new_workspace->set_fullscreen_view(nullptr);
        }
        new_workspace->fullscreen_view = OptionalRef(view.get());
        new_workspace->arrange_workspace();
    }

    bool floating = workspace.is_view_floating(view.get());
    if (workspace.is_view_floating(view.get()) && new_workspace->output.has_value() && view->expansion_state != View::ExpansionState::FULLSCREEN) {
        auto output_area = server->output_manager.get_output_real_usable_area(new_workspace->output.raw_pointer());

        if (view->x < output_area.x || view->x >= output_area.x + output_area.width || view->y < output_area.y || view->y >= output_area.y + output_area.height) {
            view->move(
                output_area.x + output_area.width / 2,
                output_area.y + output_area.height / 2);
        }
    }

    workspace.remove_view(view.get());
    new_workspace->add_view(view.get(), nullptr, floating);

    if (auto last_focused_view = std::find_if(server->seat.focus_stack.begin(), server->seat.focus_stack.end(), [workspace, view](View* v) {
            return v->workspace_id == workspace.index && v != view.get();
        });
        last_focused_view != server->seat.focus_stack.end()) {
        server->seat.focus_view(server.get(), *last_focused_view);
    } else {
        server->seat.focus_view(server.get(), nullptr);
    };
    cursor_rebase(*server, server->seat, server->seat.cursor);
}

static void validate_output(NotNullPointer<Server> server, NotNullPointer<View> view)
{
    if (server->workspaces[view->workspace_id].find_floating(view.get()) != server->workspaces[view->workspace_id].floating_views.end()) {

        Output* current_output;
        if (auto optional_output = server->output_manager.get_output_at(view->x, view->y); optional_output) {
            current_output = optional_output.raw_pointer();
        } else {
            return;
        }

        if (current_output && current_output != server->workspaces[view->workspace_id].output.raw_pointer()) {
            auto workspace = std::find_if(server->workspaces.begin(), server->workspaces.end(), [current_output](auto& w) {
                return w.output.raw_pointer() == current_output;
            });

            change_view_workspace(server, view, &(*workspace));
        }
    }
}

void reconfigure_view_position(NotNullPointer<Server> server, NotNullPointer<View> view, int x, int y)
{
    if (auto& workspace = server->workspaces[view->workspace_id]; workspace.find_tile(view.get()) != workspace.tiles.end()) {
        int dx = view->x - x;

        scroll_workspace(&workspace, RelativeScroll { dx });
    } else {
        view->move(x, y);
        validate_output(server, view);
    }
}

void reconfigure_view_size(NotNullPointer<Server> server, NotNullPointer<View> view, int width, int height)
{
    auto& workspace = server->workspaces[view->workspace_id];

    if (workspace.find_tile(view.get()) != workspace.tiles.end()) {
        height = view->geometry.height;
    }

    view->resize(width, height);
}

void scroll_workspace(NotNullPointer<Workspace> workspace, AbsoluteScroll scroll)
{
    workspace->scroll_x = scroll.get();
    workspace->arrange_workspace();
}

void scroll_workspace(NotNullPointer<Workspace> workspace, RelativeScroll scroll)
{
    workspace->scroll_x += scroll.get();
    workspace->arrange_workspace();
}
