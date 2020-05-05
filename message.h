#ifndef MESSAGE_H
#define MESSAGE_H

#include <memory>
#include <string.h>
#include <arpa/inet.h>
#include <functional>

#include "common.h"

enum class MessageKind
{
    message,
    subscribe,
    leave,
    who,
    set_echoing,
    set_processing,
    server_response,
    close_connection
};

class Message;

using raw_data = std::vector<uint8_t>;
using receive_callback = std::function<void(const Message&)>;

class Message
{
public:
    Message(unsigned int client_id, MessageKind kind);
    Message(unsigned int client_id, MessageKind kind, std::string value);
    Message(unsigned int client_id, MessageKind kind, bool value);
    ~Message();
    static Message from_raw_data(const raw_data &value, unsigned int client_id = 0);
    raw_data bytes() const;
    unsigned int client_id() const;
    MessageKind kind() const;
    operator std::string() const;
    operator bool() const;

private:
    const unsigned int client_id_;
    const MessageKind kind_;
    const union argument
    {
        argument(std::string value) {
            string = strdup(value.c_str());
            if (!string) {
                throw std::runtime_error("unable to copy message");
            }
        }
        argument(bool value) : enabled(value) {};

        char* string;
        bool enabled;
    } argument_;
};

#endif // MESSAGE_H
