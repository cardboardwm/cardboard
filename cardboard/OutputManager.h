#ifndef __CARDBOARD_OUTPUT_MANAGER_H_
#define __CARDBOARD_OUTPUT_MANAGER_H_

extern "C" {
#include <wlr/types/wlr_output_layout.h>
}

#include <list>

#include "NotNull.h"
#include "OptionalRef.h"

struct Output;

struct OutputManager {
    struct wlr_output_layout* output_layout;
    std::list<Output> outputs;

    /// Returns the box of an output in the output layout.
    NotNullPointer<const struct wlr_box> get_output_box(NotNullPointer<const Output>) const;

    /// Returns the usable area of the \a output as a rectangle with its coordinates placed in the global (output layout) space.
    struct wlr_box get_output_real_usable_area(NotNullPointer<const Output>) const;

    /// Returns the output under the given point, if any.
    OptionalRef<Output> get_output_at(double lx, double ly) const;

    /// Returns true if the \a reference output contains the given point.
    bool output_contains_point(NotNullPointer<const Output> reference, int lx, int ly) const;
};

#endif // __CARDBOARD_OUTPUT_MANAGER_H_
