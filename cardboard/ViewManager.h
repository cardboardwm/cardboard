#ifndef CARDBOARD_VIEW_MANAGER_H_INCLUDED
#define CARDBOARD_VIEW_MANAGER_H_INCLUDED

#include <list>
#include <memory>

#include "View.h"
#include "Workspace.h"

struct ViewManager {
    std::list<std::unique_ptr<View>> views;

    /// Common mapping procedure for views regardless of their underlying shell.
    void map_view(Server&, View&);
    /// Common unmapping procedure for views regardless of their underlying shell.
    void unmap_view(Server&, View&);
    /// Puts the \a view on top.
    void move_view_to_front(View&);

    Workspace& get_views_workspace(Server&, View&);
};

#endif // CARDBOARD_VIEW_MANAGER_H_INCLUDED
