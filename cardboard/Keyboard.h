// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_KEYBOARD_H_INCLUDED
#define CARDBOARD_KEYBOARD_H_INCLUDED

extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_input_device.h>
}

#include <unordered_map>

#include "Command.h"
#include "NotNull.h"

/**
 * \file
 * \brief This file has event handlers for keyboard-related events and an implementation
 * of key bindings.
 */

struct Server;
struct Seat;

/// A keyboard device, managed by a seat.
struct Keyboard {
    struct wlr_input_device* device;

private:
    // i can't make a friend only the required method (Seat::add_keyboard) because
    // that would be a cyclic dependency
    friend struct Seat;
};

/**
 * \brief This structure holds the configured key bindings.
 *
 * Key bindings are an association between a sequence of pressed modifiers and one normal key on the keyboard,
 * and an IPC command to call when the keys are pressed together.
 *
 * With \c cutter, binding a terminal to <tt>alt+return</tt> (Alt key and Enter):
 *
 * \code{.sh}
 * cutter bind alt+return exec terminal
 * \endcode
 *
 * Is equivalent to calling the following command when Alt and Return are pressed together:
 *
 * \code{.sh}
 * cutter exec terminal
 * \endcode
 */
struct KeybindingsConfig {
    static_assert(WLR_MODIFIER_COUNT <= 12, "too many modifiers");

    /**
     * \brief An array of key binding maps, for each modifier combination.
     *
     * The array index is the mod mask of the key binding.
     *
     * They key of each map is the lowercase variant of the keysym, and the value is
     * a pair of an IPC command handler and its arguments.
     *
     * \attention The \c keysym \b must be the lowercase variant! Key bindings containing uppercase characters
     * will have the shift mod mask set.
     *
     * For example, to retrieve the IPC command for the <tt>super + shift + x</tt> binding:
     *
     * \code{.cpp}
     * map[WLR_MODIFIER_LOGO | WLR_MODIFIER_SHIFT][XKB_KEY_x] // notice the lowercase `x`
     * \endcode
     */
    std::unordered_map<xkb_keysym_t, Command> map[(1 << WLR_MODIFIER_COUNT) - 1];
};

/**
 * \brief Object that is passed to keyboard-related handlers for context.
 */
struct KeyboardHandleData {
    NotNullPointer<Seat> seat; ///< The seat of the device
    NotNullPointer<Keyboard> keyboard; ///< The device from which the key handling event arised
    NotNullPointer<KeybindingsConfig> config; ///< Pointer to the global key binding configuration

    bool operator==(KeyboardHandleData other) const
    {
        return keyboard == other.keyboard && config == other.config;
    }

private:
    /**
      * \brief Fired when a non-modifier key is pressed.
      *
      * Executes key binding commands if the key together with the currently active
      * modifiers match. Else, sends the key to the surface currently holding keyboard focus.
      */
    static void key_handler(struct wl_listener* listener, void* data);

    /// Signals that a keyboard has been disconnected.
    static void destroy_handler(struct wl_listener* listener, void* data);

    /// Notifies the currently focused surface about the pressed state of the modifier keys.
    static void modifiers_handler(struct wl_listener* listener, void* data);

    friend void register_keyboard_handlers(Server& server, Seat& seat, Keyboard& keyboard);
};

void register_keyboard_handlers(Server& server, Seat& seat, Keyboard& keyboard);

#endif // CARDBOARD_KEYBOARD_H_INCLUDED
