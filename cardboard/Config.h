// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
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
    } focus_color { 0.f, 0.f, 0.7f, 0.5f };
};

#endif // CARDBOARD_CONFIG_H_INCLUDED
