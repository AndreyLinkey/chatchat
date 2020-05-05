#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

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

class client_handler
{
public:
    client_handler(int client_fd, unsigned int timeout, unsigned int client_id, receive_callback on_message_receive);
    void run();
    void exit();
    void send(const Message& message);
    unsigned int id() const;
    bool terminated() const;
    const std::vector<std::string>& groups() const;
    bool in_group(const std::string& group) const;
    bool subscribe(const std::string& group);
    bool leave(const std::string& group);
    bool echoing_required() const;
    void set_echoing(bool value);
    bool processing_required() const;
    void set_processing(bool value);

private:
    unsigned int read_data();

    const int client_fd_;
    const unsigned int timeout_;
    raw_data data_buff_;
    const unsigned int client_id_;
    const std::function<void(const Message&)> on_receive_;
    bool terminate_;
    std::vector<std::string> groups_;
    bool echoing_msg_;
    bool processing_msg_;
};

#endif // CLIENT_HANDLER_H
