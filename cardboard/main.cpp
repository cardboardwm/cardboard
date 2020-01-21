#include <wlr_cpp/util/log.h>

#include "Server.h"

int main()
{
    wlr_log_init(WLR_DEBUG, nullptr);
    Server server {};

    server.run();
    server.stop();
    return 0;
}
