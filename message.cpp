#include "message.h"

Message::Message(unsigned client_id, MessageKind kind)
    : client_id_(client_id), kind_(kind), argument_(false)
{
    if (kind_ != MessageKind::who) {
        throw std::runtime_error("unable to create message without value");
    }
}

Message::Message(unsigned client_id, MessageKind kind, std::string value)
    : client_id_(client_id), kind_(kind), argument_(value)
{
    if (kind_ != MessageKind::message && kind_ != MessageKind::subscribe && kind_ != MessageKind::leave) {
        free(argument_.string);
        throw std::runtime_error("unable to set value for given message kind");
    }
}

Message::Message(unsigned client_id, MessageKind kind, bool enabled)
    : client_id_(client_id), kind_(kind), argument_(enabled)
{
    if (kind_ != MessageKind::set_echoing && kind_ != MessageKind::set_processing) {
        throw std::runtime_error("unable to set value for given message kind");
    }

}
Message::~Message()
{
    if (kind_ == MessageKind::message || kind_ == MessageKind::subscribe || kind_ == MessageKind::leave) {
        free(argument_.string);
    }
}

Message Message::from_raw_data(const raw_data &value, unsigned client_id)
{
    if (value.size() < MIN_DATA_LEN) {
        throw std::runtime_error("message is too short");
    }
    MessageKind kind = static_cast<MessageKind>(value[0]);
    if (kind != MessageKind::server_response && client_id == 0) {
        throw std::runtime_error("invalid client id value for this message kind");
    }

    switch (kind) {
    case MessageKind::message:
    case MessageKind::subscribe:
    case MessageKind::leave:
    {
        std::string value;
        if(value.size() > MIN_DATA_LEN) {
            value = std::string(value[MIN_DATA_LEN], value.size() - MIN_DATA_LEN);
        }
        return Message(client_id, kind, value);
    }
    case MessageKind::who:
    case MessageKind::close_connection:
        return Message(client_id, kind);

    case MessageKind::set_echoing:
    case MessageKind::set_processing:
    {
        bool enabled = false;
        if(value.size() > MIN_DATA_LEN) {
            enabled = value[MIN_DATA_LEN] > 0;
        }
        return Message(client_id, kind, enabled);
    }
    case MessageKind::server_response:
    {
        unsigned msg_beg = MIN_DATA_LEN + sizeof(uint32_t);
        if(value.size() < msg_beg) {
            throw std::runtime_error("unable to get client id");
        }
        uint32_t client_id = static_cast<uint32_t>(value[0]) << 24 |
                             static_cast<uint32_t>(value[1]) << 16 |
                             static_cast<uint32_t>(value[2]) << 8  |
                             static_cast<uint32_t>(value[3]);
        client_id = ntohl(client_id);
        std::string value;
        if(value.size() > msg_beg) {
            value = std::string(value[msg_beg], value.size() - msg_beg);
        }
        return Message(client_id, kind, value);
    }
    default:
        throw std::runtime_error("unknown message kind");
    }

}

Message::operator raw_data() const
{
    raw_data data(MIN_DATA_LEN);
    data[0] = static_cast<uint8_t>(kind_);

    switch (kind_) {
    case MessageKind::message:
    case MessageKind::subscribe:
    case MessageKind::leave:
    {
        std::string value(argument_.string);
        data.insert(data.end(), value.begin(), value.end());
        break;
    }
    case MessageKind::who:
    case MessageKind::close_connection:
        break;

    case MessageKind::set_echoing:
    case MessageKind::set_processing:
        data.push_back(uint8_t(argument_.enabled));
        break;
    case MessageKind::server_response:
    {
        uint32_t net_id = htonl(client_id_);
        data.insert(data.end(), sizeof(uint32_t), 0);
        data[1] = (net_id >> 24) & 0xFF;
        data[2] = (net_id >> 16) & 0xFF;
        data[3] = (net_id >> 8) & 0xFF;
        data[4] = net_id & 0xFF;
        std::string value(argument_.string);
        data.insert(data.end(), value.begin(), value.end());
        break;
    }
    default:
        throw std::runtime_error("unknown message kind");
    }
    return data;
}

Message::operator std::string() const
{
    if (kind_ != MessageKind::message && kind_ != MessageKind::subscribe && kind_ != MessageKind::leave) {
        throw std::runtime_error("unable to get string value for message");
    }
    return std::string(argument_.string);
}

Message::operator bool() const
{
    if (kind_ != MessageKind::set_echoing && kind_ != MessageKind::set_processing) {
        throw std::runtime_error("unable to get bool value for message");
    }
    return argument_.enabled;
}
