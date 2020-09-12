extern "C" {
#include <wayland-server-core.h>

#define static
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/render/wlr_renderer.h>
#undef static

#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include "Table.h"
#include "TypeId.h"
#include "Listener.h"

struct View
{
    int x, y;
    int mapped;
};

struct ViewInternal
{
    wlr_xdg_surface* xdg_surface;
};

using ViewId = TypeId<View>;
using ViewTable = Table<ViewId, View, ViewInternal>;

int main()
{
    return 0;
}