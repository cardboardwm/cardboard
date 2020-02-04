#ifndef __CARDBOARD_KEYBOARD_H_
#define __CARDBOARD_KEYBOARD_H_

#include <wayland-server.h>

struct Server;

struct Keyboard {
    Server* server;
    struct wlr_input_device* device;

    struct wl_listener modifiers;
    struct wl_listener key;

    Keyboard(Server* server, struct wlr_input_device* device);
    Keyboard(const Keyboard&) = delete;


};

void modifiers_handler(struct wl_listener* listener, void* data);
void key_handler(struct wl_listener* listener, void* data);

void create_keyboard(Server* server, wlr_input_device*);

#endif // __CARDBOARD_KEYBOARD_H_
