#ifndef __CARDBOARD_CONFIG_H_
#define __CARDBOARD_CONFIG_H_

#include <cstdint>

/// Various config keys for the compositor.
struct Config {
    /**
     * \brief One or more keys that when pressed allow the user to move and/or resize
     * the window interactively.
     *
     * The config key in cutter is named 'mouse_mod' for simplicity, yet it allows more
     * if you want.
     * */
    uint32_t mouse_mods;
};

#endif // __CARDBOARD_CONFIG_H_
