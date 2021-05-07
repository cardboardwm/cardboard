---
title: cutter
section: 1
header: Cardboard Manual
footer: Version 0.0.1
---

# NAME
cutter - the Cardboard client that sends commands to the compositor

# SYNOPSIS
*cutter* COMMAND [ARGUMENTS]

# COMMANDS
cutter *bind* KEYBIND COMMAND 
:   Registers KEYBIND to execute COMMAND when the key binding is activated

cutter *exec* COMMAND
:   Executes COMMAND in Cardboard

cutter *config* (mouse_mod|gap|focus_color) VALUES
:   Configures a feature in Cardboard. *mouse_mod* takes a keyboard key as VALUE, 
    gap takes a number of pixels as VALUE, and focus_color takes three numbers representing 
    a colour as VALUES

cutter *quit*
:   Terminates Cardboard Compositor execution

cutter *focus* (left|right|up|down|cycle)
:   Changes the currently focused window

cutter *close*
:   Terminates the window of the currently active application

cutter *workspace* (move|switch) N
:   move option moves the currently active window to workspace number N. 
    switch option switches the active workspace to workspace number N.

cutter *toggle_floating*
:   Toggles current window between tiled and floating states.

cutter *move* DX [DY]
:   If the active window is floating, it is moved left and right into the chain of 
    tiles depending on the sign of DX, and optionally up and down in the column 
    depending on the sign of DY. If the current window is floating, it is moved
    by DX and DY (in pixels) from its original position

cutter *resize* width height
:   Resizes currently active window

cutter *pop_from_column*
:   Pops the active window from the column it is in, into a new one.


# ENVIRONMENT
*CARDBOARD_SOCKET*
The IPC unix domain socket address accepting commands

# SEE ALSO
*cardboard(1)*

# BUGS
See GitLab issues: https://gitlab.com/cardboardwm/cardboard/-/issues

# LICENSE
Copyright (C) 2020-2021 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see <https://www.gnu.org/licenses/>.

