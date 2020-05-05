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

    try {
        Server srv(DEFAULT_PORT);
        srv.begin_accept(terminate);
    } catch (const std::exception &e) {
        std::cerr << "server error: " << e.what() << std::endl;
    }


    return 0;
}
