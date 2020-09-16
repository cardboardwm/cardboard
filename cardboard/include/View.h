#ifndef UTIL_VIEW_H
#define UTIL_VIEW_H

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

#endif //UTIL_VIEW_H
