#ifndef CARDBOARD_OUTPUT_MANAGER_H_INCLUDED
#define CARDBOARD_OUTPUT_MANAGER_H_INCLUDED

extern "C" {
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
}

#include <list>
#include <memory>
#include <vector>

#include "NotNull.h"
#include "OptionalRef.h"
#include "Workspace.h"

struct Output;
struct Server;

struct OutputManager {
    wlr_output_manager_v1* output_manager_v1;
    wlr_output_layout* output_layout;
    std::list<Output> outputs;
    std::vector<Workspace> workspaces;

    void register_handlers(Server& server, struct wl_signal* new_output);

    /// Returns the box of an output in the output layout.
    NotNullPointer<const struct wlr_box> get_output_box(const Output& output) const;

    /// Returns the usable area of the \a output as a rectangle with its coordinates placed in the global (output layout) space.
    struct wlr_box get_output_real_usable_area(const Output& output) const;

    /// Returns the output under the given point, if any.
    OptionalRef<Output> get_output_at(double lx, double ly) const;

    /// Returns true if the \a reference output contains the given point.
    bool output_contains_point(const Output& reference, int lx, int ly) const;

    /// Removes \a output from the output list. Doesn't do anything else.
    void remove_output_from_list(Output& output);

    /// Creates a new workspace, without any assigned output.
    Workspace& create_workspace(Server* server);
    Workspace& get_view_workspace(View&);

    static void output_manager_apply_handler(wl_listener* listener, void* data);

    static void output_manager_test_handler(wl_listener* listener, void* data);

private:
    /**
    * \brief Executed when a new output (monitor) is attached.
    *
    * It adds the output to \a output_layout. The output
    * will be processed further after \a output_layout signals
    * that the output has been added and configured.
    *
    * \sa output_layout_add_handler Called after \a OutputManager::output_layout adds and configures the output.
    */
    static void new_output_handler(struct wl_listener* listener, void* data);

    /**
    * \brief Processes the output after it has been configured by \a output_layout.
    *
    * After that, the compositor stores it and registers some event handlers.
    * The compositor then assigns a workspace to this output, creating one if none is available.
    */
    static void output_layout_add_handler(struct wl_listener* listener, void* data);
};

using OutputManagerInstance = std::unique_ptr<OutputManager>;

OutputManagerInstance create_output_manager(Server* server);

#endif // CARDBOARD_OUTPUT_MANAGER_H_INCLUDED
