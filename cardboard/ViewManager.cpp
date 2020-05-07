#include "ViewManager.h"

void change_view_workspace(NotNullPointer<Server> server, NotNullPointer<View> view, NotNullPointer<Workspace> new_workspace)
{
    Workspace& workspace = server->workspaces[view->workspace_id];

    workspace.remove_view(static_cast<View*>(view));
    new_workspace->add_view(static_cast<View*>(view), nullptr);
}


void reconfigure_view(NotNullPointer<Server> server, NotNullPointer<View> view, wlr_box logical_geometry)
{
    if(!view->get_views_output(server.get()).has_value()) {
        return;
    }

    view->x = logical_geometry.x;
    view->y = logical_geometry.y;
    view->resize(logical_geometry.width, logical_geometry.height);

    if(
        server->workspaces[view->workspace_id].find_floating(view.get()) !=
            server->workspaces[view->workspace_id].floating_views.end() ) {
        auto* current_output = static_cast<Output*>(wlr_output_layout_output_at(server->output_layout, view->x, view->y)->data);

        if(current_output != server->workspaces[view->workspace_id].output.raw_pointer()) {
            auto workspace = std::find_if(server->workspaces.begin(), server->workspaces.end(), [current_output](auto& w){
                return  w.output.raw_pointer() == current_output;
            });

            change_view_workspace(server, view, &(*workspace));
        }
    }
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
    workspace->arrange_tiles();
}

void scroll_workspace(NotNullPointer<Workspace> workspace, RelativeScroll scroll)
{
    workspace->scroll_x += scroll.get();
    workspace->arrange_tiles();
}
