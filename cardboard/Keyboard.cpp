#include <wayland-server.h>
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

    uint32_t modifiers = wlr_keyboard_get_modifiers(handleData.device->keyboard);
    if (event->state == WLR_KEY_PRESSED)
    {
        const xkb_keysym_t* syms;
        int syms_number = xkb_state_key_get_syms(
            handleData.device->keyboard->xkb_state,
            event->keycode + 8,
            &syms
        );

        for(int i = 0; i < syms_number; i++)
        {
            auto& map = handleData.config->map[modifiers];
            if(auto it = map.find(syms[i]); it != map.end())
                it->second(server);
        }
    }

    wlr_seat_set_keyboard(server->seat, handleData.device);
    wlr_seat_keyboard_notify_key(server->seat, event->time_msec, event->keycode, event->state);
}
