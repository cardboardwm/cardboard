#include <wayland-server.h>
#include <wlr_cpp/backend.h>
#include <wlr_cpp/backend/multi.h>
#include <wlr_cpp/types/wlr_input_device.h>

#include "Keyboard.h"
#include "Server.h"

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
    auto handleData = get_listener_data<KeyHandleData>(listener);

    auto* event = static_cast<struct wlr_event_keyboard_key*>(data);

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(handleData.device->keyboard);
    if (event->state == WLR_KEY_PRESSED) {
        const xkb_keysym_t* syms;
        int syms_number = xkb_state_key_get_syms(
            handleData.device->keyboard->xkb_state,
            event->keycode + 8,
            &syms);

        for (int i = 0; i < syms_number; i++) {
            auto& map = handleData.config->map[modifiers];
            // as you can see below, keysyms are always stored lowercase
            if (auto it = map.find(xkb_keysym_to_lower(syms[i])); it != map.end()) {
                (it->second.first)(it->second.second, server);
                handled = true;
            }
        }

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
        wlr_seat_set_keyboard(server->seat, handleData.device);
        wlr_seat_keyboard_notify_key(server->seat, event->time_msec, event->keycode, event->state);
    }
}

void keyboard_destroy_handler(struct wl_listener* listener, [[maybe_unused]] void* data)
{
    Server* server = get_server(listener);
    auto device = get_listener_data<wlr_input_device*>(listener);

    server->listeners.clear_listeners(device);
    server->listeners.clear_listeners(KeyHandleData { device, &server->keybindings_config });

    server->keyboards.erase(std::remove_if(server->keyboards.begin(), server->keyboards.end(), [device](auto* other) {
                                return device == other;
                            }),
                            server->keyboards.end());
}
