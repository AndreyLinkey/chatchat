#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <system_error>
#include <thread>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <mutex>
#include <algorithm>

#include "client_handler.h"
#include "message.h"
#include "common.h"

class Server
{
public:
    Server(unsigned short port, bool default_echoing, bool default_processing);
    void begin_accept(bool &terminate_flag);
    void message_received(const Message &message);

private:
    using client_handler_ptr = std::unique_ptr<client_handler>;
    using client_thread_ptr = std::unique_ptr<std::thread>;

    int prepare_socket(unsigned short port);
    void accept_connection();
    void send_message(unsigned int recipient, unsigned int sender, const std::string& message) const;
    void send_message(const std::vector<unsigned int>& recipients, unsigned int sender, const std::string &message) const;
    void cleanup_terminated();
    std::vector<unsigned int> subscribed_handlers(const std::vector<std::string>& groups) const;
    std::string process_message(const std::string& message);

    std::mutex processing_;
    int socket_fd_;
    unsigned int new_client_idx_;
    const bool default_echoing_;
    const bool default_processing_;
    std::map<unsigned int, client_handler_ptr> clients_;
    std::map<unsigned int, client_thread_ptr> threads_;

};

#endif // SERVER_H
