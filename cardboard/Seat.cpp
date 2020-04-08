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

void init_seat(Server* server, Seat* seat, const char* name)
{
    seat->wlr_seat = wlr_seat_create(server->wl_display, name);

    seat->cursor = SeatCursor {};
    init_cursor(server, seat, &seat->cursor);

    struct {
        wl_signal* signal;
        wl_notify_func_t notify;
    } to_add_listeners[] = {
        { &seat->wlr_seat->events.request_set_cursor, seat_request_cursor_handler },
        { &seat->wlr_seat->events.request_set_selection, seat_request_selection_handler },
        { &seat->wlr_seat->events.set_primary_selection, seat_request_primary_selection_handler },
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
        View* to_focus = nullptr;
        for (View* v : focus_stack) {
            if (v == view || !v->mapped) {
                continue;
            }

            if (auto ws = server->get_views_workspace(v); ws && ws->get().output) {
                to_focus = v;
                break;
            }
        }

        focus_view(server, to_focus);
    }
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
    if (prev_view) {
        // deactivate previous surface
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

    if (auto ws = server->get_views_workspace(view)) {
        ws->get().fit_view_on_screen(view);
    }
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
    if (!ws) {
        return;
    }

    auto it = ws->get().find_tile(focused_view);
    if (int index = std::distance(ws->get().tiles.begin(), it) + offset; index < 0 || index >= static_cast<int>(ws->get().tiles.size())) {
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

void Seat::begin_interactive(View* view, GrabState::Mode mode, uint32_t edges)
{
    assert(grab_state == std::nullopt);
    struct wlr_surface* focused_surface = wlr_seat->pointer_state.focused_surface;
    if (view->get_surface() != focused_surface) {
        // don't handle the request if the view is not in focus
        return;
    }
    struct wlr_box geometry = view->geometry;

    GrabState state = { mode, view, 0, 0, geometry.width, geometry.height, edges };
    if (mode == GrabState::Mode::MOVE) {
        state.x = cursor.wlr_cursor->x - view->x;
        state.y = cursor.wlr_cursor->y - view->y;
    } else {
        state.x = cursor.wlr_cursor->x;
        state.y = cursor.wlr_cursor->y;
    }
    grab_state = state;
}

void Seat::process_cursor_motion(Server* server, uint32_t time)
{
    if (grab_state.has_value() && grab_state->mode == GrabState::Mode::MOVE) {
        process_cursor_move();
        return;
    }
    if (grab_state.has_value() && grab_state->mode == GrabState::Mode::RESIZE) {
        process_cursor_resize();
        return;
    }
    double sx, sy;
    struct wlr_surface* surface = nullptr;
    server->get_surface_under_cursor(cursor.wlr_cursor->x, cursor.wlr_cursor->y, surface, sx, sy);
    if (!surface) {
        // set the cursor to default
        wlr_xcursor_manager_set_cursor_image(cursor.wlr_cursor_manager, "left_ptr", cursor.wlr_cursor);
    }
    if (surface && is_input_allowed(surface)) {
        bool focus_changed = wlr_seat->pointer_state.focused_surface != surface;
        // Gives pointer focus when the cursor enters the surface
        wlr_seat_pointer_notify_enter(wlr_seat, surface, sx, sy);
        if (!focus_changed) {
            // the enter event contains coordinates, so we notify on motion only
            // if the focus did not change
            wlr_seat_pointer_notify_motion(wlr_seat, time, sx, sy);
        }
    } else {
        // Clear focus so pointer events are not sent to the last entered surface
        wlr_seat_pointer_clear_focus(wlr_seat);
    }
}

void Seat::process_cursor_move()
{
    assert(grab_state.has_value());
    grab_state->view->x = cursor.wlr_cursor->x - grab_state->x;
    grab_state->view->y = cursor.wlr_cursor->y - grab_state->y;
}

void Seat::process_cursor_resize()
{
    assert(grab_state.has_value());
    double dx = cursor.wlr_cursor->x - grab_state->x;
    double dy = cursor.wlr_cursor->y - grab_state->y;
    double x = grab_state->view->x;
    double y = grab_state->view->y;
    double width = grab_state->width;
    double height = grab_state->height;
    if (grab_state->resize_edges & WLR_EDGE_TOP) {
        y = grab_state->y + dy;
        height -= dy;
        if (height < 1) {
            y += height;
        }
    } else if (grab_state->resize_edges & WLR_EDGE_BOTTOM) {
        height += dy;
    }
    if (grab_state->resize_edges & WLR_EDGE_LEFT) {
        x = grab_state->x + dx;
        width -= dx;
        if (width < 1) {
            x += width;
        }
    } else if (grab_state->resize_edges & WLR_EDGE_RIGHT) {
        width += dx;
    }
    grab_state->view->x = x;
    grab_state->view->y = y;
    grab_state->view->resize(width, height);
}

void Seat::end_interactive()
{
    grab_state = std::nullopt;
}

std::optional<std::reference_wrapper<Workspace>> Seat::get_focused_workspace(Server* server)
{
    for (auto& ws : server->workspaces) {
        if (ws.output && wlr_output_layout_contains_point(server->output_layout, (*ws.output)->wlr_output, cursor.wlr_cursor->x, cursor.wlr_cursor->y)) {
            return ws;
        }
    }

    return std::nullopt;
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

void seat_request_cursor_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);

    auto* event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
    struct wlr_seat_client* focused_client = server->seat.wlr_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(server->seat.cursor.wlr_cursor, event->surface, event->hotspot_x, event->hotspot_y);
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
