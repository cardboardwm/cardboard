#ifndef __CARDBOARD_KEYBOARD_H_
#define __CARDBOARD_KEYBOARD_H_

extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_input_device.h>
}

#include <unordered_map>

#include "Command.h"

/**
 * \file
 * \brief This file has event handlers for keyboard-related events and an implementation
 * of key bindings.
 */

struct Server;
struct Seat;

/// A keyboard device, managed by a seat.
struct Keyboard {
    Seat* seat; ///< The seat that manages this keyboard.
    struct wlr_input_device* device;

    /// Signals that a keyboard has been disconnected.
    static void destroy_handler(struct wl_listener* listener, void* data);

    /// Notifies the currently focused surface about the pressed state of the modifier keys.
    static void modifiers_handler(struct wl_listener* listener, void* data);
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
 * \brief Object that is passed to \c key_handler for context.
 */
struct KeyHandleData {
    Keyboard* keyboard; ///< The device from which the key handling event arised
    KeybindingsConfig* config; ///< Pointer to the global key binding configuration

    bool operator==(KeyHandleData other)
    {
        return keyboard == other.keyboard && config == other.config;
    }
};

/**
 * \brief Fired when a non-modifier key is pressed.
 *
 * Executes key binding commands if the key together with the currently active
 * modifiers match. Else, sends the key to the surface currently holding keyboard focus.
 */
void key_handler(struct wl_listener* listener, void* data);

#endif // __CARDBOARD_KEYBOARD_H_
