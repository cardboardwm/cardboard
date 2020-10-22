// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "Spawn.h"

template <typename... Args>
static void ignore(Args&&...)
{
}

std::error_code spawn(std::function<int()> fn)
{
    // credits: http://www.lubutu.com/code/spawning-in-unix
    int fd[2];
    if (pipe(fd) == -1) {
        int err = errno;
        return std::error_code(err, std::generic_category());
    }
    if (fork() == 0) {
        close(fd[0]);
        fcntl(fd[1], F_SETFD, FD_CLOEXEC);

        setsid();
        int status = fn();

        ignore(write(fd[1], &errno, sizeof(errno)));
        exit(status);
    }

    close(fd[1]);

    int code = 0;
    if (!read(fd[0], &code, sizeof(code))) {
        code = 0;
    }
    close(fd[0]);
    return std::error_code(code, std::generic_category());
}
