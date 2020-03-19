#ifndef __CARDBOARD_SPAWN_H_
#define __CARDBOARD_SPAWN_H_

#include <functional>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>

std::error_code spawn(std::function<int()> fn);

#endif // __CARDBOARD_SPAWN_H_
