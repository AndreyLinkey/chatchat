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
#include <atomic>
#include <map>

#include "common.h"
#include "message.h"

const static std::map<MessageKind, std::string> commands {
    {MessageKind::subscribe, "join"},
    {MessageKind::leave, "leave"},
    {MessageKind::who, "who"},
    {MessageKind::set_echoing, "echo"},
    {MessageKind::set_processing, "process"},
    {MessageKind::close_connection, "close"}
};

using terminate_callback = std::function<void(void)>;

class Client
{
public:
    Client(const std::string& ip_address, unsigned short port, unsigned int timeout, terminate_callback on_terminate);
    void run();
    void terminate();
    void send_message(const std::string& message);

private:
    int prepare_socket(const char* ip_address, unsigned short port);
    int read_data();
    void message_received(const Message &message);
    void send(const Message& message);

    int socket_fd_;
    const unsigned int timeout_;
    raw_data data_buff_;
    const terminate_callback terminate_callback_;
    std::atomic<bool> terminate_;
};

#endif // CLIENT_H
