#ifndef __CARDBOARD_KEYBOARD_H_
#define __CARDBOARD_KEYBOARD_H_

#include <wayland-server.h>
#include <wlr_cpp/types/wlr_input_device.h>

#include <unordered_map>

struct Server;

using KeybindingCallback = void (*)(Server*);

struct KeybindingsConfig {
    static_assert(WLR_MODIFIER_COUNT <= 12, "too many modifiers");
    std::unordered_map<xkb_keysym_t, KeybindingCallback> map[(1 << WLR_MODIFIER_COUNT) - 1];
};

struct KeyHandleData {
    wlr_input_device* device;
    KeybindingsConfig* config;
};

void modifiers_handler(struct wl_listener* listener, void* data);
void key_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_KEYBOARD_H_
