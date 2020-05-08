extern "C" {
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/util/log.h>
}

#include <functional>
#include <memory>
#include <optional>

#include "Cursor.h"
#include "Listener.h"
#include "Seat.h"
#include "Server.h"
#include "ViewManager.h"

void init_seat(Server* server, Seat* seat, const char* name)
{
    seat->wlr_seat = wlr_seat_create(server->wl_display, name);
    seat->wlr_seat->data = seat;

    seat->cursor = SeatCursor {};
    init_cursor(server, seat, &seat->cursor);

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &seat->wlr_seat->events.request_set_cursor, seat_request_cursor_handler },
        { &seat->wlr_seat->events.request_set_selection, seat_request_selection_handler },
        { &seat->wlr_seat->events.request_set_primary_selection, seat_request_primary_selection_handler },
    };

    for (const auto& to_add_listener : to_add_listeners) {
        server->listeners.add_listener(to_add_listener.signal,
                                       Listener { to_add_listener.notify, server, seat });
    }
}

void Seat::add_input_device(Server* server, struct wlr_input_device* device)
{
    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        add_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        add_pointer(server, device);
        break;
    default:
        // TODO: touchscreens and drawing tablets
        break;
    }

    // set pointer capability even if there is no mouse attached
    uint32_t capabilities = WL_SEAT_CAPABILITY_POINTER;
    if (!keyboards.empty()) {
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(wlr_seat, capabilities);
}

void Seat::add_keyboard(Server* server, struct wlr_input_device* device)
{
    keyboards.push_back(Keyboard { this, device });
    auto& keyboard = keyboards.back();
    keyboard.device->data = &keyboard;

    struct xkb_rule_names rules = {};
    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    server->listeners.add_listener(&device->keyboard->events.modifiers,
                                   Listener { modifiers_handler, server, &keyboard });
    server->listeners.add_listener(
        &device->keyboard->events.key,
        Listener { key_handler, server, KeyHandleData { &keyboard, &server->keybindings_config } });
    server->listeners.add_listener(&device->keyboard->events.destroy,
                                   Listener { keyboard_destroy_handler, server, &keyboard });
    wlr_seat_set_keyboard(wlr_seat, device);
}

void Seat::add_pointer([[maybe_unused]] Server* server, struct wlr_input_device* device)
{
    wlr_cursor_attach_input_device(cursor.wlr_cursor, device);
}

View* Seat::get_focused_view()
{
    if (wlr_seat->keyboard_state.focused_surface == nullptr) {
        return nullptr;
    }
    if (focus_stack.empty()) {
        return nullptr;
    }

    return focus_stack.front();
}

void Seat::hide_view(Server* server, View* view)
{
    // focus last focused window mapped to an active workspace
    if (get_focused_view() == view && !focus_stack.empty()) {
        auto to_focus = std::find_if(focus_stack.begin(), focus_stack.end(), [view](auto* v) -> bool {
            return v != view && v->mapped;
        });
        if (to_focus != focus_stack.end()) {
            focus_view(server, *to_focus);
        }
    }
}

void Seat::focus_surface([[maybe_unused]] Server* server, struct wlr_surface* surface)
{
    keyboard_notify_enter(surface);
}

void Seat::focus_view(Server* server, View* view)
{
    if (focused_layer) {
        auto* layer = *focused_layer;
        // this is to exchange the keyboard focus
        focus_layer(server, nullptr);
        focus_view(server, view);
        focus_layer(server, layer);
        return;
    }
    // unfocus the currently focused view
    View* prev_view = get_focused_view();
    if (prev_view == view) {
        return; // already focused
    }

    if (view) {
        auto& ws = server->get_views_workspace(view);
        // deny setting focus to a view which is hidden by a fullscreen view
        if (ws.fullscreen_view && ws.fullscreen_view.raw_pointer() != view) {
            // unless it's transient for the fullscreened view
            if (!view->is_transient_for(ws.fullscreen_view.raw_pointer())) {
                return;
            }
        }
    }

    if (prev_view) {
        // deactivate previous surface
        prev_view->close_popups();
        prev_view->set_activated(false);
        wlr_seat_keyboard_clear_focus(wlr_seat);
    }

    // if the view is null, then focus_view will only
    // unfocus the previously focused one
    if (view == nullptr) {
        return;
    }

    if (!is_input_allowed(view->get_surface())) {
        return;
    }

    // put view at the beginning of the focus stack
    if (auto it = std::find(focus_stack.begin(), focus_stack.end(), view); it != focus_stack.end()) {
        focus_stack.splice(focus_stack.begin(), focus_stack, it);
    } else {
        focus_stack.push_front(view);
    }

    // move the view to the front
    server->move_view_to_front(view);
    // activate surface
    view->set_activated(true);
    // the seat will send keyboard events to the view automatically
    keyboard_notify_enter(view->get_surface());

    // noop if the view is floating
    server->get_views_workspace(view).fit_view_on_screen(view);
}

void Seat::focus_layer(Server* server, struct wlr_layer_surface_v1* layer)
{
    if (!layer && focused_layer) {
        focused_layer = std::nullopt;
        auto* focused_view = get_focused_view();
        if (focused_view) {
            focus_view(server, nullptr); // to clear focus on the layer
            focus_view(server, focused_view);
        }
        return;
    } else if (!layer || focused_layer == layer) {
        return;
    }
    assert(layer->mapped);

    keyboard_notify_enter(layer->surface);
    if (layer->current.layer >= ZWLR_LAYER_SHELL_V1_LAYER_TOP) {
        focused_layer = layer;
    }
}

void Seat::focus_by_offset(Server* server, int offset)
{
    if (offset == 0 || get_focused_view() == nullptr) {
        return;
    }

    auto* focused_view = get_focused_view();
    if (focused_view == nullptr) {
        return;
    }
    auto ws = server->get_views_workspace(focused_view);
    if (ws.is_view_floating(focused_view)) {
        return;
    }

    auto it = ws.find_tile(focused_view);
    if (int index = std::distance(ws.tiles.begin(), it) + offset; index < 0 || index >= static_cast<int>(ws.tiles.size())) {
        // out of bounds
        return;
    }

    std::advance(it, offset);
    focus_view(server, it->view);
}

void Seat::remove_from_focus_stack(View* view)
{
    focus_stack.remove(view);
}

void Seat::begin_move(Server*, View* view)
{
    assert(grab_state == std::nullopt);
    struct wlr_surface* focused_surface = wlr_seat->pointer_state.focused_surface;
    if (view->get_surface() != focused_surface) {
        // don't handle the request if the view is not in pointer focus
        return;
    }

    grab_state = {
        .view = view,
        .grab_data = GrabState::Move {
            .lx = cursor.wlr_cursor->x,
            .ly = cursor.wlr_cursor->y,
            .view_x = view->x,
            .view_y = view->y,
        },
    };
}

void Seat::begin_resize(Server* server, View* view, uint32_t edges)
{
    assert(grab_state == std::nullopt);
    struct wlr_surface* focused_surface = wlr_seat->pointer_state.focused_surface;
    if (view->get_surface() != focused_surface) {
        // don't handle the request if the view is not in pointer focus
        return;
    }

    auto& workspace = server->get_views_workspace(view);
    grab_state = {
        .view = view,
        .grab_data = GrabState::Resize {
            .lx = cursor.wlr_cursor->x,
            .ly = cursor.wlr_cursor->y,
            .geometry = view->geometry,
            .resize_edges = edges,
            .workspace = workspace,
            .scroll_x = workspace.scroll_x,
            .view_x = view->x,
            .view_y = view->y,
        },
    };
}

void Seat::begin_workspace_scroll(Server* server, Workspace* workspace)
{
    end_interactive(server);

    grab_state = {
        .view = nullptr,
        .grab_data = GrabState::WorkspaceScroll {
            .workspace = workspace,
            .speed = 0,
            .delta_since_update = 0,
            .scroll_x = static_cast<double>(workspace->scroll_x),
            .ready = false,
            .wants_to_stop = false,
        },
    };
}

void Seat::process_cursor_motion(Server* server, uint32_t time)
{
    if (grab_state) {
        std::visit(
            [this, server](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, GrabState::Move>) {
                    process_cursor_move(server, arg);
                } else if constexpr (std::is_same_v<T, GrabState::Resize>) {
                    process_cursor_resize(server, arg);
                }
            },
            grab_state->grab_data);
        return;
    }
    cursor.rebase(server, time);
}

void Seat::process_cursor_move(Server* server, GrabState::Move move_data)
{
    assert(grab_state.has_value());

    double dx = cursor.wlr_cursor->x - move_data.lx;
    double dy = cursor.wlr_cursor->y - move_data.ly;

    View* view = grab_state->view;

    reconfigure_view_position(server, view, move_data.view_x + dx, move_data.view_y + dy);
}

void Seat::process_cursor_resize(Server* server, GrabState::Resize resize_data)
{
    assert(grab_state.has_value());

    double dx = cursor.wlr_cursor->x - resize_data.lx;
    double dy = cursor.wlr_cursor->y - resize_data.ly;
    double x = grab_state->view->x;
    double y = grab_state->view->y;
    double width = resize_data.geometry.width;
    double height = resize_data.geometry.height;

    if (resize_data.resize_edges & WLR_EDGE_TOP) {
        if (height - dy > 1) {
            y = resize_data.view_y + dy;
            height -= dy;
        }
    } else if (resize_data.resize_edges & WLR_EDGE_BOTTOM) {
        height += dy;
    }
    if (resize_data.resize_edges & WLR_EDGE_LEFT) {
        if (width - dx > 1) {
            x = resize_data.view_x + dx;
            width -= dx;
        }
    } else if (resize_data.resize_edges & WLR_EDGE_RIGHT) {
        width += dx;
    }

    reconfigure_view_position(server, grab_state->view, x, y);
    reconfigure_view_size(server, grab_state->view, width, height);
}

void Seat::process_swipe_begin(Server* server, uint32_t fingers)
{
    end_touchpad_swipe(server);
    if (fingers == WORKSPACE_SCROLL_FINGERS) {
        get_focused_workspace(server).and_then([this, server](auto& ws) {
            begin_workspace_scroll(server, &ws);
        });
    }
}

void Seat::process_swipe_update(Server* server, uint32_t fingers, double dx, double)
{
    GrabState::WorkspaceScroll* data;
    if (!grab_state.has_value() || !(data = std::get_if<GrabState::WorkspaceScroll>(&grab_state->grab_data))) {
        return;
    }
    if (fingers < WORKSPACE_SCROLL_FINGERS) {
        end_touchpad_swipe(server);
        return;
    }

    data->delta_since_update += dx * WORKSPACE_SCROLL_SENSITIVITY;
    data->ready = true;
}

void Seat::process_swipe_end(Server*)
{
    GrabState::WorkspaceScroll* data;
    if (!grab_state.has_value() || !(data = std::get_if<GrabState::WorkspaceScroll>(&grab_state->grab_data))) {
        return;
    }

    data->wants_to_stop = true;
    wlr_log(WLR_DEBUG, "fingers were lifted - swipe stopping");
}

void Seat::end_interactive(Server* server)
{
    grab_state = std::nullopt;
    cursor.rebase(server);
}

/**
 * \brief During a touchpad swipe event, for some reason, we don't get a swipe end event if we lift less than the three fingers.
 * So we have to check for this situation in scrolling (usually happens with two fingers) and cursor moving (one finger) events.
 */
void Seat::end_touchpad_swipe(Server* server)
{
    if (grab_state && std::holds_alternative<Seat::GrabState::WorkspaceScroll>(grab_state->grab_data)) {
        end_interactive(server);
        wlr_log(WLR_DEBUG, "stopped swipe");
    }
}

void Seat::update_swipe(Server* server)
{
    GrabState::WorkspaceScroll* data;
    if (!grab_state.has_value() || !(data = std::get_if<GrabState::WorkspaceScroll>(&grab_state->grab_data))) {
        return;
    }

    if (!data->ready) {
        return;
    }

    if (data->delta_since_update != 0) {
        data->speed = data->delta_since_update;
        data->delta_since_update = 0;
    }

    data->scroll_x -= data->speed;
    scroll_workspace(data->workspace, AbsoluteScroll { static_cast<int>(data->scroll_x) });

    data->speed *= WORKSPACE_SCROLL_FRICTION;

    if (data->wants_to_stop && fabs(data->speed) < 1) {
        end_touchpad_swipe(server);
    }
}

OptionalRef<Workspace> Seat::get_focused_workspace(Server* server)
{
    for (auto& ws : server->workspaces) {
        if (ws.output && wlr_output_layout_contains_point(server->output_layout, ws.output.unwrap().wlr_output, cursor.wlr_cursor->x, cursor.wlr_cursor->y)) {
            return ws;
        }
    }

    return OptionalRef<Workspace>(nullptr);
}

void Seat::keyboard_notify_enter(struct wlr_surface* surface)
{
    auto* keyboard = wlr_seat_get_keyboard(wlr_seat);
    if (!keyboard) {
        wlr_seat_keyboard_notify_enter(wlr_seat, surface, nullptr, 0, nullptr);
        return;
    }

    wlr_seat_keyboard_notify_enter(wlr_seat, surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void Seat::set_exclusive_client(Server* server, struct wl_client* client)
{
    // deactivate inhibition
    if (!client) {
        exclusive_client = std::nullopt;

        auto* raw_output_under_cursor = wlr_output_layout_output_at(server->output_layout, cursor.wlr_cursor->x, cursor.wlr_cursor->y);
        if (raw_output_under_cursor != nullptr) {
            auto* output_under_cursor = static_cast<Output*>(raw_output_under_cursor->data);
            arrange_layers(server, output_under_cursor);
        }
        return;
    }

    // if the currently focused layer is not the client, remove focus
    if (focused_layer && wl_resource_get_client((*focused_layer)->resource) != client) {
        focus_layer(server, nullptr);
    }

    // if the currently focused view is not the client, remove focus
    if (auto* focused_view = get_focused_view(); focused_view != nullptr && wl_resource_get_client(focused_view->get_surface()->resource) != client) {
        focus_view(server, nullptr);
    }

    // if the seat has pointer focus on someone who is not the client, remove focus
    if (wlr_seat->pointer_state.focused_client != nullptr && wlr_seat->pointer_state.focused_client->client != client) {
        wlr_seat_pointer_clear_focus(wlr_seat);
    }

    exclusive_client = client;
}

bool Seat::is_input_allowed(struct wlr_surface* surface)
{
    auto* client = wl_resource_get_client(surface->resource);
    return !exclusive_client || *exclusive_client == client;
}

bool Seat::is_mod_pressed(uint32_t mods)
{
    auto* keyboard = wlr_seat_get_keyboard(wlr_seat);
    if (!keyboard) {
        return false;
    }

    return (wlr_keyboard_get_modifiers(keyboard) & mods) == mods;
}

void Seat::focus(Server* server, Workspace* workspace)
{
    if (workspace == get_focused_workspace(server).raw_pointer()) {
        return;
    }

    if (!workspace->output.has_value()) {
        Workspace& previous_workspace = get_focused_workspace(server).unwrap();
        workspace->activate(previous_workspace.output.unwrap());
        previous_workspace.deactivate();

        Output& output = workspace->output.unwrap();

        cursor.move(
            server,
            output.usable_area.x + output.usable_area.width / 2,
            output.usable_area.y + output.usable_area.height / 2);
    }

    if (auto last_focused_view = std::find_if(focus_stack.begin(), focus_stack.end(), [workspace](View* view) {
            return view->workspace_id == workspace->index;
        });
        last_focused_view != focus_stack.end()) {
        focus_view(server, *last_focused_view);
    } else {
        focus_view(server, nullptr);
    }
    server->seat.cursor.rebase(server);
}

void seat_request_cursor_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);

    auto* event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
    struct wlr_seat_client* focused_client = seat->wlr_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client && !seat->grab_state) {
        server->seat.cursor.set_image_surface(server, event->surface, event->hotspot_x, event->hotspot_y);
    }
}

void seat_request_selection_handler(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "normal selection happening");
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<wlr_seat_request_set_selection_event*>(data);

    wlr_seat_set_selection(seat->wlr_seat, event->source, event->serial);
}

void seat_request_primary_selection_handler(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "primary selection happening");
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<wlr_seat_request_set_primary_selection_event*>(data);

    wlr_seat_set_primary_selection(seat->wlr_seat, event->source, event->serial);
}
