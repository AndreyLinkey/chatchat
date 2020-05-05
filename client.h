#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <system_error>
#include <poll.h>
#include <iostream>
#include <functional>

#include "common.h"
#include "message.h"

class Client
{
public:
    Client(const std::string& ip_address, unsigned short port, unsigned int timeout, receive_callback on_message_receive);
    void run();
    void terminate();
    void send(const Message& message);

private:
    int prepare_socket(const char* ip_address, unsigned short port);
    int read_data();

    int socket_fd_;
    const unsigned int timeout_;
    raw_data data_buff_;
    const std::function<void(const Message&)> on_receive_;
    bool terminate_;
};

#endif // CLIENT_H
