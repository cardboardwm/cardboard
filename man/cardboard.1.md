---
title: cardboard
section: 1
header: Cardboard Manual
footer: Version 0.0.1
---

# NAME
cardboard - A scrollable tiling Wayland compositors designed with 
laptops in mind.

# SYNOPSIS
*cardboard*

# DESCRIPTION
Cardboard is a unique, scrollable tiling Wayland compositor designed with laptops
in mind, based on *wlroots*.

Scrollable tiling is an idea that comes from the Gnome extension PaperWM. 
In this tiling mode, the windows occupy the height of the screen and are displaced
in a list, such as that the user can scroll through them, as opposed to tiling
by filling the screen with windows disposed in a tree like structure. The main idea
is based on the fact that the user will most likely not look at more than three windows
at the same time.

# ENVIRONMENT
*CARDBOARD_SOCKET*
    The IPC unix domain socket address accepting commands

# SEE ALSO
*cutter(1)*

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
