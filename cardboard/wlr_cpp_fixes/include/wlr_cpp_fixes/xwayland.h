#ifndef CARDBOARD_WLR_CPP_FIXES_XWAYLAND_H_INCLUDED
#define CARDBOARD_WLR_CPP_FIXES_XWAYLAND_H_INCLUDED

extern "C" {

#define class class_
#define static
#include <wlr/xwayland.h>
#undef class
#undef static

}

#endif // CARDBOARD_WLR_CPP_FIXES_XWAYLAND_H_INCLUDED
