extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/util/log.h>
}

#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>

#include "Cursor.h"
#include "Helpers.h"
#include "Listener.h"
#include "Seat.h"
#include "Server.h"
#include "ViewOperations.h"

static void add_keyboard(Server& server, Seat& seat, struct wlr_input_device* device)
{
    seat.keyboards.push_back(Keyboard { device });
    auto& keyboard = seat.keyboards.back();
    keyboard.device->data = &keyboard;

    struct xkb_rule_names rules = {};
    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    register_keyboard_handlers(server, seat, keyboard);

    wlr_seat_set_keyboard(seat.wlr_seat, device);
}

static void add_pointer(Seat& seat, struct wlr_input_device* device)
{
    wlr_cursor_attach_input_device(seat.cursor.wlr_cursor, device);
}

static void add_input_device(Server& server, Seat& seat, struct wlr_input_device* device)
{
    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        add_keyboard(server, seat, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        add_pointer(seat, device);
        break;
    default:
        // TODO: touchscreens and drawing tablets
        break;
    }

    // set pointer capability even if there is no mouse attached
    uint32_t capabilities = WL_SEAT_CAPABILITY_POINTER;
    if (!seat.keyboards.empty()) {
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(seat.wlr_seat, capabilities);
}

void init_seat(Server& server, Seat& seat, const char* name)
{
    seat.wlr_seat = wlr_seat_create(server.wl_display, name);
    seat.wlr_seat->data = &seat;

    seat.cursor = SeatCursor {};
    init_cursor(server.output_manager, seat.cursor);

    seat.inhibit_manager = wlr_input_inhibit_manager_create(server.wl_display);

    register_handlers(server, &seat, {
                                         { &seat.wlr_seat->events.request_set_cursor, Seat::request_cursor_handler },
                                         { &seat.wlr_seat->events.request_set_selection, Seat::request_selection_handler },
                                         { &seat.wlr_seat->events.request_set_primary_selection, Seat::request_primary_selection_handler },

                                         { &seat.cursor.wlr_cursor->events.motion, Seat::cursor_motion_handler },
                                         { &seat.cursor.wlr_cursor->events.motion_absolute, Seat::cursor_motion_absolute_handler },
                                         { &seat.cursor.wlr_cursor->events.button, Seat::cursor_button_handler },
                                         { &seat.cursor.wlr_cursor->events.axis, Seat::cursor_axis_handler },
                                         { &seat.cursor.wlr_cursor->events.frame, Seat::cursor_frame_handler },

                                         { &seat.cursor.wlr_cursor->events.swipe_begin, Seat::cursor_swipe_begin_handler },
                                         { &seat.cursor.wlr_cursor->events.swipe_update, Seat::cursor_swipe_update_handler },
                                         { &seat.cursor.wlr_cursor->events.swipe_end, Seat::cursor_swipe_end_handler },
                                     });
}

void Seat::register_handlers(Server& server, struct wl_signal* new_input)
{
    ::register_handlers(server,
                        this,
                        {
                            { new_input, Seat::new_input_handler },
                            { &inhibit_manager->events.activate, Seat::activate_inhibit_handler },
                            { &inhibit_manager->events.deactivate, Seat::deactivate_inhibit_handler },
                        });
}

OptionalRef<View> Seat::get_focused_view()
{
    if (wlr_seat->keyboard_state.focused_surface == nullptr) {
        return NullRef<View>;
    }
    if (focus_stack.empty()) {
        return NullRef<View>;
    }

    return OptionalRef<View>(focus_stack.front());
}

void Seat::hide_view(Server& server, View& view)
{
    // focus last focused window mapped to an active workspace
    if (get_focused_view().raw_pointer() == &view && !focus_stack.empty()) {
        auto to_focus = std::find_if(focus_stack.begin(), focus_stack.end(), [&view](NotNullPointer<View> v) -> bool {
            return v != &view && v->mapped;
        });
        if (to_focus != focus_stack.end()) {
            focus_view(server, **to_focus, true);
        }
    }
}

void Seat::focus_surface(struct wlr_surface* surface)
{
    keyboard_notify_enter(surface);
}

void Seat::focus_view(Server& server, OptionalRef<View> view, bool condense_workspace)
{
    if (focused_layer) {
        auto* layer = *focused_layer;
        // this is to exchange the keyboard focus
        focus_layer(server, nullptr);
        focus_view(server, view, condense_workspace);
        focus_layer(server, layer);
        return;
    }
    // unfocus the currently focused view
    auto prev_view = get_focused_view();
    if (prev_view == view) {
        if (!view) {
            return;
        }
        goto fit_on_screen;
    }

    if (view) {
        auto& ws = server.output_manager.get_view_workspace(view.unwrap());
        // deny setting focus to a view which is hidden by a fullscreen view
        if (ws.fullscreen_view && ws.fullscreen_view != view) {
            // unless it's transient for the fullscreened view
            if (!view.unwrap().is_transient_for(ws.fullscreen_view.unwrap())) {
                return;
            }
        }
    }

    if (prev_view) {
        // deactivate previous surface
        prev_view.unwrap().close_popups();
        prev_view.unwrap().set_activated(false);
        wlr_seat_keyboard_clear_focus(wlr_seat);
    }

    // if the view is null, then focus_view will only
    // unfocus the previously focused one
    if (!view) {
        return;
    }

    {
        auto& view_r = view.unwrap();
        if (!is_input_allowed(view_r.get_surface())) {
            return;
        }

        // put view at the beginning of the focus stack
        if (auto it = std::find(focus_stack.begin(), focus_stack.end(), &view_r); it != focus_stack.end()) {
            focus_stack.splice(focus_stack.begin(), focus_stack, it);
        } else {
            focus_stack.push_front(&view_r);
        }

        // move the view_r to the front
        server.surface_manager.move_view_to_front(view_r);
        // activate surface
        view_r.set_activated(true);
        // the seat will send keyboard events to the view automatically
        keyboard_notify_enter(view_r.get_surface());
    }

fit_on_screen:
    // noop if the view is floating
    server.output_manager.get_view_workspace(view.unwrap()).fit_view_on_screen(server.output_manager, view.unwrap(), condense_workspace);
}

void Seat::focus_layer(Server& server, struct wlr_layer_surface_v1* layer)
{
    if (!layer && focused_layer) {
        focused_layer = std::nullopt;
        auto focused_view = get_focused_view();
        if (focused_view) {
            focus_view(server, NullRef<View>); // to clear focus on the layer
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

void Seat::focus_column(Server& server, Workspace::Column& column)
{
    // focus the most recently focused view of the column
    auto column_views = column.get_mapped_and_normal_set();
    for (auto view_ptr : focus_stack) {
        if (column_views.contains(view_ptr)) {
            focus_view(server, OptionalRef(view_ptr));
            break;
        }
    }
}

void Seat::remove_from_focus_stack(View& view)
{
    focus_stack.remove(&view);
}

void Seat::begin_move(Server& server, View& view)
{
    assert(grab_state == std::nullopt);
    struct wlr_surface* focused_surface = wlr_seat->pointer_state.focused_surface;
    if (view.get_surface() != focused_surface) {
        // don't handle the request if the view is not in pointer focus
        return;
    }

    auto& workspace = server.output_manager.get_view_workspace(view);
    auto floating_it = std::find(workspace.floating_views.begin(), workspace.floating_views.end(), &view);
    bool is_tiled = floating_it == workspace.floating_views.end();

    if (is_tiled) {
        for (auto& column : workspace.columns) {
            for(auto& tile: column.tiles) {
                server.view_animation->cancel_tasks(*tile.view);
            }
        }
    }

    grab_state = {
        .grab_data = GrabState::Move {
            .view = &view,
            .lx = cursor.wlr_cursor->x,
            .ly = cursor.wlr_cursor->y,
            .view_x = view.x,
            .view_y = view.y,
        },
    };
}

void Seat::begin_resize(Server& server, View& view, uint32_t edges)
{
    assert(grab_state == std::nullopt);
    struct wlr_surface* focused_surface = wlr_seat->pointer_state.focused_surface;
    if (view.get_surface() != focused_surface) {
        // don't handle the request if the view is not in pointer focus
        return;
    }

    auto& workspace = server.output_manager.get_view_workspace(view);
    grab_state = {
        .grab_data = GrabState::Resize {
            .view = &view,
            .lx = cursor.wlr_cursor->x,
            .ly = cursor.wlr_cursor->y,
            .geometry = view.geometry,
            .resize_edges = edges,
            .workspace = workspace,
            .scroll_x = workspace.scroll_x,
            .view_x = view.x,
            .view_y = view.y,
        },
    };
}

void Seat::begin_workspace_scroll(Server& server, Workspace& workspace)
{
    end_interactive(server);

    grab_state = {
        .grab_data = GrabState::WorkspaceScroll {
            .workspace = &workspace,
            .dominant_view = get_focused_view(),
            .speed = 0,
            .delta_since_update = 0,
            .scroll_x = static_cast<double>(workspace.scroll_x),
            .ready = false,
            .wants_to_stop = false,
        },
    };
}

void Seat::process_cursor_motion(Server& server, uint32_t time)
{
    if (grab_state) {
        std::visit(
            [this, &server](auto&& arg) {
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
    cursor_rebase(server, *this, cursor, time);
}

void Seat::process_cursor_move(Server& server, GrabState::Move move_data)
{
    assert(grab_state.has_value());

    double dx = cursor.wlr_cursor->x - move_data.lx;
    double dy = cursor.wlr_cursor->y - move_data.ly;

    View* view = move_data.view;

    reconfigure_view_position(server, *view, move_data.view_x + dx, move_data.view_y + dy, false);
}

void Seat::process_cursor_resize(Server& server, GrabState::Resize resize_data)
{
    assert(grab_state.has_value());

    double dx = cursor.wlr_cursor->x - resize_data.lx;
    double dy = cursor.wlr_cursor->y - resize_data.ly;
    double x = resize_data.view->x;
    double y = resize_data.view->y;
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

    reconfigure_view_position(server, *resize_data.view, x, y);
    auto column_it = resize_data.workspace.unwrap().find_column(resize_data.view);
    if (column_it == resize_data.workspace.unwrap().columns.end()) {
        reconfigure_view_size(server, *resize_data.view, width, height);
    } else {
        // resize all views in the column
        for (auto& tile : column_it->tiles) {
            if (tile.view->is_mapped_and_normal()) {
                reconfigure_view_size(server, *tile.view, width, tile.view->geometry.height);
            }
        }
    }
}

void Seat::process_swipe_begin(Server& server, uint32_t fingers)
{
    end_touchpad_swipe(server);
    if (fingers == WORKSPACE_SCROLL_FINGERS) {
        get_focused_workspace(server).and_then([this, &server](auto& ws) {
            begin_workspace_scroll(server, ws);
        });
    }
}

void Seat::process_swipe_update(Server& server, uint32_t fingers, double dx)
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

void Seat::process_swipe_end(Server&)
{
    GrabState::WorkspaceScroll* data;
    if (!grab_state.has_value() || !(data = std::get_if<GrabState::WorkspaceScroll>(&grab_state->grab_data))) {
        return;
    }

    data->wants_to_stop = true;
    wlr_log(WLR_DEBUG, "fingers were lifted - swipe stopping");
}

void Seat::end_interactive(Server& server)
{
    grab_state = std::nullopt;
    cursor_rebase(server, *this, cursor);
}

/**
 * \brief During a touchpad swipe event, for some reason, we don't get a swipe end event if we lift less than the three fingers.
 * So we have to check for this situation in scrolling (usually happens with two fingers) and cursor moving (one finger) events.
 */
void Seat::end_touchpad_swipe(Server& server)
{
    if (grab_state && std::holds_alternative<Seat::GrabState::WorkspaceScroll>(grab_state->grab_data)) {
        end_interactive(server);
        wlr_log(WLR_DEBUG, "stopped swipe");
    }
}

void Seat::update_swipe(Server& server)
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
    scroll_workspace(server.output_manager, *data->workspace, AbsoluteScroll { static_cast<int>(data->scroll_x) });
    data->workspace->find_dominant_view(server.output_manager, *this, get_focused_view()).and_then([data](auto& dominant) {
        data->dominant_view = OptionalRef(dominant);
    });
    if (data->dominant_view) {
        get_focused_view().and_then([](auto& view) {
            view.set_activated(false);
        });
        data->dominant_view.unwrap().set_activated(true);
    }

    data->speed *= WORKSPACE_SCROLL_FRICTION;

    if (data->wants_to_stop && fabs(data->speed) < 1) {
        focus_view(server, OptionalRef(data->dominant_view));
        end_touchpad_swipe(server);
    }
}

bool Seat::is_grabbing(View& view)
{
    if (!grab_state) {
        return false;
    }

    bool result = false;
    std::visit([&view, &result](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, GrabState::Move> || std::is_same_v<T, GrabState::Resize>) {
            result = &view == arg.view;
        } else {
            result = false;
        }
    },
               grab_state->grab_data);

    return result;
}

OptionalRef<Workspace> Seat::get_focused_workspace(Server& server)
{
    for (auto& ws : server.output_manager.workspaces) {
        if (ws.output && server.output_manager.output_contains_point(ws.output.unwrap(), cursor.wlr_cursor->x, cursor.wlr_cursor->y)) {
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

void Seat::set_exclusive_client(Server& server, struct wl_client* client)
{
    // deactivate inhibition
    if (!client) {
        exclusive_client = std::nullopt;

        server.output_manager.get_output_at(cursor.wlr_cursor->x, cursor.wlr_cursor->y).and_then([&server](auto& output_under_cursor) {
            arrange_layers(server, output_under_cursor);
        });
        return;
    }

    // if the currently focused layer is not the client, remove focus
    if (focused_layer && wl_resource_get_client((*focused_layer)->resource) != client) {
        focus_layer(server, nullptr);
    }

    // if the currently focused view is not the client, remove focus
    if (auto focused_view = get_focused_view(); focused_view.has_value() && wl_resource_get_client(focused_view.unwrap().get_surface()->resource) != client) {
        focus_view(server, NullRef<View>);
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

void Seat::focus(Server& server, Workspace& workspace)
{
    if (&workspace == get_focused_workspace(server).raw_pointer()) {
        return;
    }

    if (!workspace.output.has_value()) {
        Workspace& previous_workspace = get_focused_workspace(server).unwrap();
        workspace.activate(previous_workspace.output.unwrap());
        previous_workspace.deactivate();
    }

    const Output& output = workspace.output.unwrap();
    const struct wlr_box* output_box = server.output_manager.get_output_box(output);

    cursor_warp(
        server,
        *this,
        cursor,
        output_box->x + output.usable_area.x + output.usable_area.width / 2,
        output_box->y + output.usable_area.y + output.usable_area.height / 2);

    if (auto last_focused_view = std::find_if(focus_stack.begin(), focus_stack.end(), [&workspace](View* view) {
            return view->workspace_id == workspace.index;
        });
        last_focused_view != focus_stack.end()) {
        focus_view(server, OptionalRef<View>(*last_focused_view));
    } else {
        focus_view(server, NullRef<View>);
    }
    cursor_rebase(server, *this, cursor);
}

void Seat::new_input_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);

    auto* device = static_cast<struct wlr_input_device*>(data);

    add_input_device(*server, *seat, device);
}

void Seat::activate_inhibit_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);

    seat->set_exclusive_client(*server, seat->inhibit_manager->active_client);
}

void Seat::deactivate_inhibit_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);

    seat->set_exclusive_client(*server, nullptr);
}

void Seat::request_cursor_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);

    auto* event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
    struct wlr_seat_client* focused_client = seat->wlr_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client && !seat->grab_state) {
        cursor_set_image_surface(*server, *seat, seat->cursor, event->surface, event->hotspot_x, event->hotspot_y);
    }
}

void Seat::request_selection_handler(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "normal selection happening");
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<wlr_seat_request_set_selection_event*>(data);

    wlr_seat_set_selection(seat->wlr_seat, event->source, event->serial);
}

void Seat::request_primary_selection_handler(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "primary selection happening");
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<wlr_seat_request_set_primary_selection_event*>(data);

    wlr_seat_set_primary_selection(seat->wlr_seat, event->source, event->serial);
}

void Seat::cursor_motion_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_motion*>(data);

    // in case the user was doing a three finger swipe and lifted two fingers.
    seat->end_touchpad_swipe(*server);

    wlr_cursor_move(seat->cursor.wlr_cursor, event->device, event->delta_x, event->delta_y);
    seat->process_cursor_motion(*server, event->time_msec);
}

void Seat::cursor_motion_absolute_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(seat->cursor.wlr_cursor, event->device, event->x, event->y);
    seat->process_cursor_motion(*server, event->time_msec);
}

void Seat::cursor_button_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_button*>(data);

    if (event->state == WLR_BUTTON_RELEASED) {
        wlr_seat_pointer_notify_button(seat->wlr_seat, event->time_msec, event->button, event->state);
        // end grabbing
        seat->end_interactive(*server);
        return;
    }

    double sx, sy;
    struct wlr_surface* surface;
    auto view = server->surface_manager.get_surface_under_cursor(server->output_manager, seat->cursor.wlr_cursor->x, seat->cursor.wlr_cursor->y, surface, sx, sy);
    if (!view) {
        wlr_seat_pointer_notify_button(seat->wlr_seat, event->time_msec, event->button, event->state);
        return;
    }
    auto& view_r = view.unwrap();
    if (seat->is_mod_pressed(server->config.mouse_mods)) {
        if (event->button == BTN_LEFT) {
            cursor_set_image(*server, *seat, seat->cursor, "grab");
            seat->begin_move(*server, view_r);
            wlr_seat_pointer_clear_focus(seat->wlr_seat); // must be _after_ begin_move
        } else if (event->button == BTN_RIGHT) {
            uint32_t edge = 0;
            edge |= seat->cursor.wlr_cursor->x > view_r.x + view_r.geometry.x + view_r.geometry.width / 2 ? WLR_EDGE_RIGHT : WLR_EDGE_LEFT;
            edge |= seat->cursor.wlr_cursor->y > view_r.y + view_r.geometry.y + view_r.geometry.height / 2 ? WLR_EDGE_BOTTOM : WLR_EDGE_TOP;
            const char* image = NULL;
            if (edge == (WLR_EDGE_LEFT | WLR_EDGE_TOP)) {
                image = "nw-resize";
            } else if (edge == (WLR_EDGE_TOP | WLR_EDGE_RIGHT)) {
                image = "ne-resize";
            } else if (edge == (WLR_EDGE_RIGHT | WLR_EDGE_BOTTOM)) {
                image = "se-resize";
            } else if (edge == (WLR_EDGE_BOTTOM | WLR_EDGE_LEFT)) {
                image = "sw-resize";
            }

            cursor_set_image(*server, *seat, seat->cursor, image);
            seat->begin_resize(*server, view_r, edge);
            wlr_seat_pointer_clear_focus(seat->wlr_seat); // must be _after_ begin_resize
        }
    } else {
        wlr_seat_pointer_notify_button(seat->wlr_seat, event->time_msec, event->button, event->state);
        if (view != seat->get_focused_view()) {
            seat->focus_view(*server, view);
        }
    }
}

void Seat::cursor_axis_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_axis*>(data);

    // in case the user was doing a three finger swipe and lifted a finger
    seat->end_touchpad_swipe(*server);

    wlr_seat_pointer_notify_axis(seat->wlr_seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}

void Seat::cursor_frame_handler(struct wl_listener* listener, void*)
{
    auto* seat = get_listener_data<Seat*>(listener);

    wlr_seat_pointer_notify_frame(seat->wlr_seat);
}

void Seat::cursor_swipe_begin_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_swipe_begin*>(data);

    seat->process_swipe_begin(*server, event->fingers);
}

void Seat::cursor_swipe_update_handler(struct wl_listener* listener, void* data)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);
    auto* event = static_cast<struct wlr_event_pointer_swipe_update*>(data);

    seat->process_swipe_update(*server, event->fingers, event->dx);
}

void Seat::cursor_swipe_end_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto* seat = get_listener_data<Seat*>(listener);

    seat->process_swipe_end(*server);
}
