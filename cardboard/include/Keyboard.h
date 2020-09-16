#ifndef UTIL_KEYBOARD_H
#define UTIL_KEYBOARD_H

struct Server;

struct Keyboard
{
    wlr_input_device* input_device;

    ListenerId modifiers;
    ListenerId key;
};

Keyboard create_keyboard(Server* server, wlr_input_device *device); // since these are long living "objects", doing resource deallocation in dtor could be good;
void destroy(Server* server, Keyboard keyboard);

#endif //UTIL_KEYBOARD_H
