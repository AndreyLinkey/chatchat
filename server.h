#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <system_error>
#include <thread>
#include <memory>
#include <vector>
#include <functional>
#include <iostream>

#include "client_handler.h"
#include "common.h"

class Server
{
public:
    Server(unsigned short port);
    void begin_accept(bool &terminate_flag);

private:
    using client_handler_ptr = std::unique_ptr<client_handler>;
    using client_thread_ptr = std::unique_ptr<std::thread>;

    void prepare_socket(unsigned short port);
    void accept_connection();
    void cleanup_terminated();

    int socket_fd_;
    unsigned client_idx_;
    std::vector<client_handler_ptr> clients_;
    std::vector<client_thread_ptr> threads_;

};



#endif // SERVER_H
