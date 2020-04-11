#ifndef __CARDBOARD_XWAYLAND_FIX_H_
#define __CARDBOARD_XWAYLAND_FIX_H_

extern "C" {

#define class class_
#define static
#include <wlr/xwayland.h>
#undef class
#undef static

}

#endif // __CARDBOARD_XWAYLAND_FIX_H_
