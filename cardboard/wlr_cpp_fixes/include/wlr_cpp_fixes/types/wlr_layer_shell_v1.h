// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_WLR_CPP_FIXES_WLR_LAYER_SHELL_V1_H_INCLUDED
#define CARDBOARD_WLR_CPP_FIXES_WLR_LAYER_SHELL_V1_H_INCLUDED

extern "C" {

#define namespace namespace_
#include <wlr/types/wlr_layer_shell_v1.h>
#undef namespace
}

#endif // CARDBOARD_WLR_CPP_FIXES_WLR_LAYER_SHELL_V1_H_INCLUDED
