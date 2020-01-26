#include <wayland-server.h>
#include <wlr_cpp/types/wlr_input_device.h>

#include "Keyboard.h"
#include "Server.h"

Keyboard::Keyboard(Server* server, struct wlr_input_device* device)
    : server(server)
    , device(device)
{
    struct xkb_rule_names rules = {};
    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    modifiers.notify = Keyboard::modifiers_handler;
    wl_signal_add(&device->keyboard->events.modifiers, &modifiers);
    key.notify = Keyboard::key_handler;
    wl_signal_add(&device->keyboard->events.key, &key);

    wlr_seat_set_keyboard(server->seat, device);
}

void Keyboard::modifiers_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Keyboard* keyboard = wl_container_of(listener, keyboard, modifiers);
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
    // send modifiers to the client
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &keyboard->device->keyboard->modifiers);
}

void Keyboard::key_handler(struct wl_listener* listener, void* data)
{
    Keyboard* keyboard = wl_container_of(listener, keyboard, key);
    Server* server = keyboard->server;
    auto* event = static_cast<struct wlr_event_keyboard_key*>(data);

    // TODO: handle compositor keybinds

    wlr_seat_set_keyboard(server->seat, keyboard->device);
    wlr_seat_keyboard_notify_key(server->seat, event->time_msec, event->keycode, event->state);
}
