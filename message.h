#ifndef MESSAGE_H
#define MESSAGE_H

#include <memory>
#include <string.h>
#include <arpa/inet.h>

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

class Message
{
public:
    Message(unsigned client_id, MessageKind kind);
    Message(unsigned client_id, MessageKind kind, std::string value);
    Message(unsigned client_id, MessageKind kind, bool value);
    ~Message();
    static Message from_raw_data(const raw_data &value, unsigned client_id = 0);
    operator raw_data() const;
    operator std::string() const;
    operator bool() const;

private:
    const unsigned client_id_;
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
