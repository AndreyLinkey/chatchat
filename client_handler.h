#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <atomic>
#include <poll.h>
#include <unistd.h>
#include <system_error>
#include <iostream>

class client_handler
{
public:
    client_handler(int client_fd, unsigned timeout, unsigned client_id);
    void run();
    void exit();
    bool terminated();

private:
    unsigned read_data();

    const int client_fd_;
    std::atomic<bool> terminate_;
    const unsigned timeout_;
    const unsigned client_id_;
};

#endif // CLIENT_HANDLER_H
