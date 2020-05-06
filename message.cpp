#include "message.h"

Message::Message(unsigned int client_id, MessageKind kind)
    : client_id_(client_id), kind_(kind), argument_(false)
{
    if (kind_ != MessageKind::who && kind != MessageKind::close_connection) {
        throw std::runtime_error("unable to create message without value");
    }
}

Message::Message(unsigned int client_id, MessageKind kind, std::string value)
    : client_id_(client_id), kind_(kind), argument_(value)
{
    if (kind_ != MessageKind::message && kind_ != MessageKind::subscribe && kind_ != MessageKind::leave
            && kind != MessageKind::server_response) {
        free(argument_.string);
        throw std::runtime_error("unable to set value for given message kind");
    }
}

Message::Message(unsigned int client_id, MessageKind kind, bool enabled)
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

Message Message::from_raw_data(const raw_data &data, unsigned int client_id)
{
    if (data.size() < MIN_DATA_LEN) {
        throw std::runtime_error("message is too short");
    }
    MessageKind kind = static_cast<MessageKind>(data[0]);

    switch (kind) {
    case MessageKind::message:
    case MessageKind::subscribe:
    case MessageKind::leave:
    {
        std::string value;
        if(data.size() > MIN_DATA_LEN) {
            int len = std::distance(data.cbegin(), std::find(data.cbegin(), data.cend(), '\0'));
            value = std::string((char*)data.data() + MIN_DATA_LEN, len - MIN_DATA_LEN);
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
        if(data.size() > MIN_DATA_LEN) {
            enabled = data[MIN_DATA_LEN] > 0;
        }
        return Message(client_id, kind, enabled);
    }
    case MessageKind::server_response:
    {
        unsigned msg_beg = MIN_DATA_LEN + sizeof(uint32_t);
        if(data.size() < msg_beg) {
            throw std::runtime_error("unable to get client id");
        }
        uint32_t client_id = static_cast<uint32_t>(data[1]) << 24 |
                             static_cast<uint32_t>(data[2]) << 16 |
                             static_cast<uint32_t>(data[3]) << 8  |
                             static_cast<uint32_t>(data[4]);
        client_id = ntohl(client_id);
        std::string value;
        if(data.size() > msg_beg) {
            raw_data::const_iterator it = data.cbegin() + msg_beg;

            int len = std::distance(it, std::find(it, data.cend(), '\0'));
            value = std::string((char*)&(*it), len);
        }
        return Message(client_id, kind, value);
    }
    default:
        throw std::runtime_error("unknown message kind");
    }

}

raw_data Message::bytes() const
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
        if (client_id_ == 0)
            throw std::runtime_error("server message must contains client id");

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
    data.push_back('\0');
    return data;
}

unsigned int Message::client_id() const
{
    if (client_id_ == 0)
        throw std::runtime_error("attempted access to message client id when it is not set");
    return client_id_;
}

MessageKind Message::kind() const
{
    return kind_;
}

Message::operator std::string() const
{
    if (kind_ != MessageKind::message && kind_ != MessageKind::subscribe && kind_ != MessageKind::leave
            && kind_ != MessageKind::server_response) {
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
