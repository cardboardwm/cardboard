extern "C" {
#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/backend/multi.h>
#include <wlr/types/wlr_input_device.h>
}

#include "Keyboard.h"
#include "Server.h"

void KeyboardHandleData::destroy_handler(struct wl_listener* listener, void*)
{
    auto* server = get_server(listener);
    auto handle_data = get_listener_data<KeyboardHandleData>(listener);

    server->listeners.clear_listeners(KeyboardHandleData { handle_data.seat, handle_data.keyboard, handle_data.config });

    handle_data.seat->keyboards.erase(std::remove_if(handle_data.seat->keyboards.begin(), handle_data.seat->keyboards.end(), [&handle_data](const auto& other) {
                                          return handle_data.keyboard == &other;
                                      }),
                                      handle_data.seat->keyboards.end());
}

void KeyboardHandleData::modifiers_handler(struct wl_listener* listener, void*)
{
    auto handle_data = get_listener_data<KeyboardHandleData>(listener);

    wlr_seat_set_keyboard(handle_data.seat->wlr_seat, handle_data.keyboard->device);
    // send modifiers to the client
    wlr_seat_keyboard_notify_modifiers(handle_data.seat->wlr_seat, &handle_data.keyboard->device->keyboard->modifiers);
}

void KeyboardHandleData::key_handler(struct wl_listener* listener, void* data)
{
    Server* server = get_server(listener);
    auto handle_data = get_listener_data<KeyboardHandleData>(listener);

    auto* event = static_cast<struct wlr_event_keyboard_key*>(data);

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(handle_data.keyboard->device->keyboard);
    if (event->state == WLR_KEY_PRESSED) {
        const xkb_keysym_t* syms;
        int syms_number = xkb_state_key_get_syms(
            handle_data.keyboard->device->keyboard->xkb_state,
            event->keycode + 8,
            &syms);

        // TODO: keybinds that work when there is an exclusive client
        if (!handle_data.seat->exclusive_client) {
            for (int i = 0; i < syms_number; i++) {
                auto& map = handle_data.config->map[modifiers];
                // as you can see below, keysyms are always stored lowercase
                if (auto it = map.find(xkb_keysym_to_lower(syms[i])); it != map.end()) {
                    (it->second)(server);
                    handled = true;
                }
            }
        }

        // VT changing works even when there is an exclusive client,
        // to avoid getting locked out
        if (!handled) {
            for (int i = 0; i < syms_number; i++) {
                if (syms[i] >= XKB_KEY_XF86Switch_VT_1 && syms[i] <= XKB_KEY_XF86Switch_VT_12) {
                    if (wlr_backend_is_multi(server->backend)) {
                        if (auto* session = wlr_backend_get_session(server->backend); session) {
                            auto vt = syms[i] - XKB_KEY_XF86Switch_VT_1 + 1;
                            wlr_session_change_vt(session, vt);
                        }
                    }
                    handled = true;
                }
            }
        }
    }

    if (!handled) {
        wlr_seat_set_keyboard(handle_data.seat->wlr_seat, handle_data.keyboard->device);
        wlr_seat_keyboard_notify_key(handle_data.seat->wlr_seat, event->time_msec, event->keycode, event->state);
    }
}
