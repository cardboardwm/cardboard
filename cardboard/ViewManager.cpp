#include "ViewManager.h"

void change_view_workspace(NotNullPointer<Server> server, NotNullPointer<View> view, NotNullPointer<Workspace> new_workspace)
{
    Workspace& workspace = server->workspaces[view->workspace_id];

    if(workspace.find_tile(view.get()) != workspace.tiles.end()) {
        workspace.remove_view(static_cast<View*>(view));
        new_workspace->add_view(static_cast<View*>(view), nullptr);
    } else {
        if(new_workspace->output.has_value()) {
            auto output_area = new_workspace->output.unwrap().usable_area;

            if(view->x < output_area.x || view->x >= output_area.x + output_area.width ||
               view->y < output_area.y || view->y >= output_area.y + output_area.height) {
                view->move(
                    output_area.x + output_area.width / 2,
                    output_area.y + output_area.height / 2);
            }
        }

        workspace.remove_view(static_cast<View*>(view));
        new_workspace->add_view(static_cast<View*>(view), nullptr, true);
    }
    server->seat.cursor.rebase(server.get());
}

static void validate_output(NotNullPointer<Server> server, NotNullPointer<View> view)
{
    if(
        server->workspaces[view->workspace_id].find_floating(view.get()) !=
            server->workspaces[view->workspace_id].floating_views.end() ) {

        Output* current_output;

        if(auto wlr_output = wlr_output_layout_output_at(server->output_layout, view->x, view->y); wlr_output) {
            current_output = static_cast<Output*>(wlr_output->data);
        }
        else {
            return ;
        }

        if(current_output && current_output != server->workspaces[view->workspace_id].output.raw_pointer()) {
            auto workspace = std::find_if(server->workspaces.begin(), server->workspaces.end(), [current_output](auto& w){
                return  w.output.raw_pointer() == current_output;
            });

            change_view_workspace(server, view, &(*workspace));
        }
    }
}

void reconfigure_view_position(NotNullPointer<Server> server, NotNullPointer<View> view, int x, int y)
{
    if(auto& workspace = server->workspaces[view->workspace_id]; workspace.find_tile(view.get()) != workspace.tiles.end()) {
        int dx = view->x - x;

        scroll_workspace(&workspace, RelativeScroll{dx});
    } else {
        view->move(x, y);
        validate_output(server, view);
    }
}

void reconfigure_view_size(NotNullPointer<Server> server, NotNullPointer<View> view, int width, int height)
{
    auto& workspace = server->workspaces[view->workspace_id];

    if(workspace.find_tile(view.get()) != workspace.tiles.end()) {
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
