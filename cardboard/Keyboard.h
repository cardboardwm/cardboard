#ifndef __CARDBOARD_KEYBOARD_H_
#define __CARDBOARD_KEYBOARD_H_

#include <wayland-server.h>
#include <wlr_cpp/types/wlr_input_device.h>

struct Server;

void modifiers_handler(struct wl_listener* listener, void* data);
void key_handler(struct wl_listener* listener, void* data);

void create_keyboard(Server* server, wlr_input_device*);

#endif // __CARDBOARD_KEYBOARD_H_
