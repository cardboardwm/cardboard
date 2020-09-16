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
#include "Seat.h"
#include "View.h"

struct Server
{
    ViewTable views;
    OutputTable outputs;
    WorkspaceTable workspaces;
    ListenerList listener_list;

    Seat seat;

    ListenerId new_xdg_surface;
    ListenerId cursor_motion;

    wlr_renderer* renderer;
    wl_display* display;
    wlr_backend* backend;
    wlr_xdg_shell* xdg_shell;

    wlr_output_layout* output_layout;
    ListenerId new_output;
};

Seat create_seat(Server* server)
{
    Seat seat;

    seat.cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(seat.cursor, server->output_layout);

    seat.cursor_manager = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(seat.cursor_manager, 1);

    //seat.cursor_motion = server->listener_list.add_listener(&seat.cursor->events.motion, );
    //seat.cursor_motion_absolute = server->listener_list.add_listener(&seat.cursor->events.motion_absolute, );
    //seat.cursor_button = server->listener_list.add_listener(&seat.cursor->events.button, );
    //seat.cursor_axis = server->listener_list.add_listener(&seat.cursor->events.axis, );
    //seat.cursor_frame = server->listener_list.add_listener(&seat.cursor->events.frame, );

    //seat.new_input = server->listener_list.add_listener(&server->backend->events.new_input, )

    seat.seat = wlr_seat_create(server->display, "seat0");

    //seat.request_set_cursor = server->add_listener(&seat.seat->events.request_set_cursor, )
    //seat.request_set_selection = server->add_listener(&seat.seat->events.request_set_selection, )

    return seat;
}

void destroy(Server* server, const Seat& seat)
{

}

Keyboard create_keyboard(Server* server, wlr_input_device *device)
{
    Keyboard keyboard;

    keyboard.input_device = device;

    struct xkb_rule_names rules = { 0 };
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules,
                                                       XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    //keyboard.key = server->listener_list.add_listener(&device->keyboard->events.key, );
    //keyboard.modifiers = server->listener_list.add_listener(&device->keyboard->events.modifiers, );

    wlr_seat_set_keyboard(server->seat.seat, device);

    return keyboard;
}

void destroy(Server* server, Keyboard keyboard)
{

}


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