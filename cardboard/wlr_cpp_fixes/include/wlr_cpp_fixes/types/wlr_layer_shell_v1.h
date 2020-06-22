#ifndef CARDBOARD_WLR_CPP_FIXES_WLR_LAYER_SHELL_V1_H_INCLUDED
#define CARDBOARD_WLR_CPP_FIXES_WLR_LAYER_SHELL_V1_H_INCLUDED

extern "C" {

#define namespace namespace_
#include <wlr/types/wlr_layer_shell_v1.h>
#undef namespace

}

#endif // CARDBOARD_WLR_CPP_FIXES_WLR_LAYER_SHELL_V1_H_INCLUDED
