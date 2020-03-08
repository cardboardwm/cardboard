#include <wlr_cpp/util/log.h>

#include <signal.h>

#include "Server.h"

Server server {};

int main()
{
    wlr_log_init(WLR_DEBUG, nullptr);
    signal(SIGINT, [](int signo) {
        if (signo == SIGINT) {
            server.teardown();
        }
    });

    server.run();
    server.stop();
    return 0;
}
