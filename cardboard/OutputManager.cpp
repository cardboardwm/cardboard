extern "C" {
#include <wlr/types/wlr_output_layout.h>
#include <wlr/util/log.h>
}

#include "Helpers.h"
#include "Layers.h"
#include "Listener.h"
#include "Output.h"
#include "OutputManager.h"

void OutputManager::register_handlers(Server& server, struct wl_signal* new_output)
{
    ::register_handlers(server, this, {
                                          { new_output, OutputManager::new_output_handler },
                                          { &output_layout->events.add, OutputManager::output_layout_add_handler },
                                      });
}

NotNullPointer<const struct wlr_box> OutputManager::get_output_box(const Output& output) const
{
    return wlr_output_layout_get_box(output_layout, output.wlr_output);
}

struct wlr_box OutputManager::get_output_real_usable_area(const Output& output) const
{
    const auto* output_box = wlr_output_layout_get_box(output_layout, output.wlr_output);
    auto real_usable_area = output.usable_area;
    real_usable_area.x += output_box->x;
    real_usable_area.y += output_box->y;

    return real_usable_area;
}

OptionalRef<Output> OutputManager::get_output_at(double lx, double ly) const
{
    auto* raw_output = wlr_output_layout_output_at(output_layout, lx, ly);
    if (raw_output == nullptr) {
        return NullRef<Output>;
    }

    return OptionalRef(static_cast<Output*>(raw_output->data));
}

bool OutputManager::output_contains_point(const Output& reference, int lx, int ly) const
{
    return wlr_output_layout_contains_point(output_layout, reference.wlr_output, lx, ly);
}

void OutputManager::remove_output_from_list(Output& output)
{
    outputs.remove_if([&output](auto& other) { return &other == &output; });
}

void OutputManager::new_output_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* output = static_cast<struct wlr_output*>(data);

    // pick the monitor's preferred mode
    if (!wl_list_empty(&output->modes)) {
        struct wlr_output_mode* mode = wlr_output_preferred_mode(output);
        wlr_output_set_mode(output, mode);
        wlr_output_enable(output, true);
        if (!wlr_output_commit(output)) {
            return;
        }
    }

    // add output to the layout. add_auto arranges outputs left-to-right
    // in the order they appear.
    wlr_output_layout_add_auto(server->output_manager->output_layout, output);
}

void OutputManager::output_layout_add_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto* l_output = static_cast<struct wlr_output_layout_output*>(data);

    auto output_ = Output { .wlr_output = l_output->output };
    // FIXME: should this go in the constructor?
    wlr_output_effective_resolution(output_.wlr_output, &output_.usable_area.width, &output_.usable_area.height);
    register_output(*server, std::move(output_));

    auto& output = server->output_manager->outputs.back();

    Workspace* ws_to_assign = nullptr;
    for (auto& ws : server->output_manager->workspaces) {
        if (!ws.output) {
            ws_to_assign = &ws;
            break;
        }
    }

    if (!ws_to_assign) {
        ws_to_assign = &server->output_manager->create_workspace(server);
    }

    ws_to_assign->activate(output);
    arrange_layers(*server, output);

    // the output doesn't need to be exposed as a wayland global
    // because wlr_output_layout does it for us already
}

Workspace& OutputManager::create_workspace(Server* server)
{
    workspaces.push_back({ .server = server, .index = static_cast<Workspace::IndexType>(workspaces.size()) });
    return workspaces.back();
}

Workspace& OutputManager::get_view_workspace(View& view)
{
    return workspaces[view.workspace_id];
}

void OutputManager::output_manager_apply_handler([[maybe_unused]] wl_listener* listener, [[maybe_unused]] void* data)
{
}

void OutputManager::output_manager_test_handler([[maybe_unused]] wl_listener* listener, [[maybe_unused]] void* data)
{
}

OutputManagerInstance create_output_manager(Server* server)
{
    auto output_manager = std::make_unique<OutputManager>();

    output_manager->output_layout = wlr_output_layout_create();
    output_manager->output_manager_v1 = wlr_output_manager_v1_create(server->wl_display);

    output_manager->register_handlers(*server, &server->backend->events.new_output);

    register_handlers(
        *server,
        output_manager.get(),
        { { &output_manager->output_manager_v1->events.apply, OutputManager::output_manager_apply_handler },
          { &output_manager->output_manager_v1->events.test, OutputManager::output_manager_test_handler } });

    return output_manager;
}