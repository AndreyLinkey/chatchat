#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <atomic>
#include <poll.h>
#include <unistd.h>
#include <system_error>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <functional>

#include "common.h"
#include "message.h"

using receive_callback = std::function<void(const Message&)>;

class client_handler
{
public:
    client_handler(int client_fd, unsigned int timeout, unsigned int client_id, receive_callback on_message_receive);
    void run();
    void exit();
    void send(const Message& message);
    unsigned int id() const;
    bool terminated() const;
    std::vector<std::string> groups() const;
    bool in_group(const std::string &group) const;
    bool echoing_required() const;
    bool processing_required() const;

private:
    unsigned int read_data();
    const int client_fd_;
    const unsigned int timeout_;
    raw_data data_buff_;
    const unsigned int client_id_;
    const std::function<void(const Message&)> on_receive_;
    std::atomic<bool> terminate_;
    mutable std::mutex groups_lock_;
    std::vector<std::string> groups_;
    std::atomic<bool> echoing_msg_;
    std::atomic<bool> processing_msg_;
};

#endif // CLIENT_HANDLER_H
