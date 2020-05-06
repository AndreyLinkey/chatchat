#include <iostream>
#include <csignal>

#include "server.h"

static bool terminate = false;

void sig_handler(int signal __attribute__((unused)))
{
    terminate = true;
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, sig_handler);
    std::signal(SIGTERM, sig_handler);

    unsigned short port(DEFAULT_PORT);
    bool echoing(DEFAULT_MESSAGE_ECHOING);
    bool processing(DEFAULT_MESSAGE_PROCESSING);

    int opt;
    while((opt = getopt(argc, argv, "p:emh")) != -1)
    {
        switch(opt) {
        case 'p':
            try {
                port = static_cast<unsigned short>(std::stoi(std::string(optarg)));
            } catch (...) {
                std::cerr << "invalid value of argument -p: " << optarg << std::endl;
                return 1;
            }
            break;
        case 'e':
            echoing = true;
            break;
        case 'm':
            processing = true;
            break;
        case 'h':
            std::cout << "-p port number\n-e enable message echoing for clients\n-m enable message processing for clients"
                      << std::endl;
            return 0;
        }
    }

    try {
        Server srv(port, echoing, processing);
        std::cout << "server started" << std::endl;
        srv.begin_accept(terminate);
        std::cout << "server terminated" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "server error: " << e.what() << std::endl;
    }

    return 0;
}
