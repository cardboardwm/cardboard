#include <wlr_cpp/util/log.h>

#include <signal.h>
#include <sys/wait.h>

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

    if (!server.init_ipc1()) {
        return EXIT_FAILURE;
    }

    signal(SIGINT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGCHLD, sig_handler);
    signal(SIGPIPE, SIG_IGN);
    if (!server.load_settings()) {
        return EXIT_FAILURE;
    }

    if (!server.init()) {
        return EXIT_FAILURE;
    }
    server.init_ipc2();

    server.run();
    server.stop();
    return server.exit_code;
}
