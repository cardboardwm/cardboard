#ifndef CARDBOARD_SPAWN_H_INCLUDED
#define CARDBOARD_SPAWN_H_INCLUDED

#include <functional>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>

/**
 * \file
 * \brief This header file holds a simple utility function for spawning
 * a system utility in another process.
 */

/**
 * \brief Executes the function \a fn in a new process, in background.
 * \a fn should call a function from the \c exec(3) family.
 *
 * If any library call from \a fn encounters an error (especially the exec function),
 * it is caught by the parent and returned by this function.
 *
 * \returns The errno value if something failed, or 0 if successful.
 * */
std::error_code spawn(std::function<int()> fn);

#endif // CARDBOARD_SPAWN_H_INCLUDED
