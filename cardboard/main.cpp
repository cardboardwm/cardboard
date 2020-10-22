// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
extern "C" {
#include <wlr/util/log.h>
}

#include <signal.h>
#include <sys/wait.h>

#include "BuildConfig.h"
#include "Server.h"

Server server;

void sig_handler(int signo)
{
    if (signo == SIGCHLD) {
        signal(signo, sig_handler);
        while (waitpid(-1, 0, WNOHANG) > 0)
            ;
    } else if (signo == SIGINT || signo == SIGHUP || signo == SIGTERM) {
        server.teardown(0);
    }
}

int main()
{
    wlr_log_init(WLR_DEBUG, nullptr);

    signal(SIGINT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGCHLD, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    if (!server.init()) {
        return EXIT_FAILURE;
    }

    server.run();
    server.stop();
    return server.exit_code;
}
