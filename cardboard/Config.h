#ifndef CARDBOARD_CONFIG_H_INCLUDED
#define CARDBOARD_CONFIG_H_INCLUDED

#include <cstdint>

/// Various configurations for the compositor.
struct Config {
    /**
     * \brief One or more keys that when pressed allow the user to move and/or resize
     * the window interactively.
     *
     * The config key in cutter is named 'mouse_mod' for simplicity, yet it allows more
     * if you want.
     * */
    uint32_t mouse_mods;

    /**
     * \brief The gap between the tiled windows (measured in pixels); default is 10 pixels
     */
     int gap = 10;

     /**
      * \brief The color behind the focused column
      */
     struct {
         float r, g, b, a;
     } focus_color {0.f, 0.f, 0.7f, 0.5f};
};

#endif // CARDBOARD_CONFIG_H_INCLUDED
