extern "C" {
#include <wayland-server-core.h>

#define static
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/render/wlr_renderer.h>
#undef static

#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include "Table.h"
#include "TypeId.h"
#include "Listener.h"

struct View
{
    int x, y;
    int mapped;
};

struct ViewInternal
{
    wlr_xdg_surface* xdg_surface;
    ListenerId map, unmap, destroy, request_move, request_resize;
};

using ViewId = TypeId<View>;
using ViewTable = Table<ViewId, View, ViewInternal>;

struct OutputInternal
{
    wlr_output* output;
    ListenerId frame;
};

using OutputId = TypeId<struct Output>;
using OutputTable = Table<OutputId, OutputInternal>;

struct Workspace
{
    OutputId output;
    std::vector<ViewId> views;
};

using WorkspaceId = TypeId<Workspace>;
using WorkspaceTable = Table<WorkspaceId, Workspace>;

struct Server
{
    ViewTable views;
    OutputTable outputs;
    WorkspaceTable workspaces;

    wlr_renderer* renderer;
};

struct render_data {
    wlr_output *output;
    wlr_renderer *renderer;
    ViewId view;
    timespec *when;
};

static void output_frame(wl_listener*, void* server_, ListenerData listener_data, void* data)
{
    auto server = static_cast<Server*>(server_);
    OutputInternal output = server->outputs.query<OutputInternal>(
        static_cast<OutputId>(std::get<std::int32_t>(listener_data))
    );
    wlr_renderer* renderer = server->renderer;

    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (!wlr_output_attach_render(output.output, nullptr)) {
        return;
    }

    int width, height;
    wlr_output_effective_resolution(output.output, &width, &height);
    wlr_renderer_begin(renderer, width, height);

    float color[4] = {0.3, 0.3, 0.3, 1.0};
    wlr_renderer_clear(renderer, color);

    auto workspace_id = std::find_if(
        server->workspaces.components.begin(),
        server->workspaces.components.end(),
        [output_id = static_cast<OutputId>(std::get<std::int32_t>(listener_data))](const auto& x) {
            return std::get<Workspace>(x.second).output == output_id;
        }
    )->first; // TODO: implement better querying

    auto workspace = server->workspaces.query<Workspace>(workspace_id);

    // TODO: order views in workspace after seat focus stack

    for(const auto view_id: workspace.views) // TODO: implement better Container<ViewId> traversal
    {
        const auto& [view, view_internal] = server->views.query<View, ViewInternal>(view_id);

        if(!view.get().mapped)
        {
            continue;
        }

        render_data rdata = {
            .output = output.output,
            .renderer = renderer,
            .view = view_id,
            .when = &now
        };

        //wlr_xdg_surface_for_each_surface(view_internal.get().xdg_surface, render_surface, &rdata);
    }

    wlr_output_render_software_cursors(output.output, nullptr);
    wlr_renderer_end(renderer);
    wlr_output_commit(output.output);
}

int main()
{
    return 0;
}