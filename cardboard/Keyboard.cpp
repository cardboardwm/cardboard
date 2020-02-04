#include <wayland-server.h>
#include <wlr_cpp/types/wlr_input_device.h>

#include "Keyboard.h"
#include "Server.h"

void create_keyboard(Server* server, wlr_input_device* device)
{
    struct xkb_rule_names rules = {};
    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    server->listeners.add_listener(&device->keyboard->events.modifiers, Listener{modifiers_handler, server, device});
    server->listeners.add_listener(&device->keyboard->events.key, Listener{key_handler, server, device});
    wlr_seat_set_keyboard(server->seat, device);
}

void modifiers_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Server* server = get_server(listener);
    auto device = get_listener_data<wlr_input_device*>(listener);

    wlr_seat_set_keyboard(server->seat, device);
    // send modifiers to the client
    wlr_seat_keyboard_notify_modifiers(server->seat, &device->keyboard->modifiers);
}

void key_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto device = get_listener_data<wlr_input_device*>(listener);

    auto* event = static_cast<struct wlr_event_keyboard_key*>(data);

    // TODO: handle compositor keybinds

    wlr_seat_set_keyboard(server->seat, device);
    wlr_seat_keyboard_notify_key(server->seat, event->time_msec, event->keycode, event->state);
}
