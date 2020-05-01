#include <iostream>
#include <csignal>

#include "server.h"

static bool terminate = false;

void sig_handler(int signal __attribute__((unused)))
{
    terminate = true;
}

int main()
{
    std::signal(SIGINT, sig_handler);
    std::signal(SIGTERM, sig_handler);

    Server srv = Server(8888);

    srv.begin_accept(terminate);
    return 0;
}
