#include "ViewManager.h"
#include "Server.h"

void ViewManager::map_view(Server& server, View& view)
{
    view.mapped = true;

    // can be nullptr, this is fine
    auto* prev_focused = server.seat.get_focused_view().raw_pointer();

    server.seat.get_focused_workspace(server).and_then([&server, &view, prev_focused](auto& ws) {
        ws.add_view(server.output_manager, view, prev_focused);
    });
    server.seat.focus_view(server, view);
}

void ViewManager::unmap_view(Server& server, View& view)
{
    if (view.mapped) {
        view.mapped = false;
        get_views_workspace(server, view).remove_view(server.output_manager, view);
    }

    server.seat.hide_view(server, view);
    server.seat.remove_from_focus_stack(view);
}

void ViewManager::move_view_to_front(View& view)
{
    views.splice(views.begin(), views, std::find_if(views.begin(), views.end(), [&view](const std::unique_ptr<View>& x) { return &view == x.get(); }));
}

Workspace& ViewManager::get_views_workspace(Server& server, View& view)
{
    assert(view.workspace_id >= 0);
    return server.workspaces[view.workspace_id];
}
