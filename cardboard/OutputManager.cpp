extern "C" {
#include <wlr/types/wlr_output_layout.h>
}

#include "Output.h"
#include "OutputManager.h"

NotNullPointer<const struct wlr_box> OutputManager::get_output_box(const NotNullPointer<const Output> output) const
{
    return wlr_output_layout_get_box(output_layout, output.get()->wlr_output);
}

struct wlr_box OutputManager::get_output_real_usable_area(NotNullPointer<const Output> output) const
{
    const auto* output_box = wlr_output_layout_get_box(output_layout, output->wlr_output);
    auto real_usable_area = output->usable_area;
    real_usable_area.x += output_box->x;
    real_usable_area.y += output_box->y;

    return real_usable_area;
}

OptionalRef<Output> OutputManager::get_output_at(double lx, double ly) const
{
    auto* raw_output = wlr_output_layout_output_at(output_layout, lx, ly);
    if (raw_output == nullptr) {
        return NullRef<Output>;
    }

    return OptionalRef(static_cast<Output*>(raw_output->data));
}

bool OutputManager::output_contains_point(NotNullPointer<const Output> reference, int lx, int ly) const
{
    return wlr_output_layout_contains_point(output_layout, reference->wlr_output, lx, ly);
}
