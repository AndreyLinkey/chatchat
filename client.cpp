#include "client.h"

Client::Client(const std::string& ip_address, unsigned short port, unsigned int timeout, terminate_callback on_terminate)
    : socket_fd_(0), timeout_(timeout), data_buff_(DATA_BUFF_LEN), terminate_callback_(on_terminate), terminate_(false)
{
    try {
        socket_fd_ = prepare_socket(ip_address.c_str(), port);
    }  catch (...) {
        if (socket_fd_) {
            close(socket_fd_);
        }
        throw;
    }
}

void Client::run()
{
    while(!terminate_) {
        int count = 0;
        try {
            count = read_data();
        } catch (const std::exception &e) {
            terminate_callback_();
            std::cerr << "error while receiving file: " << e.what() << std::endl;
            break;
        }

        switch (count) {
        case -1:
            terminate_callback_();
            terminate_ = true;
        case 0:
            break;
        default:
            try {
                Message msg = Message::from_raw_data(data_buff_, 0);
                memset(&data_buff_[0], '\0', count);
                message_received(msg);
            } catch (const std::exception &e) {
                std::cerr << "error while processing message: " << e.what() << std::endl;
            }
            break;
        }
    }
    close(socket_fd_);
}

void Client::terminate()
{
    terminate_ = true;
}

void Client::send_message(const std::string& message) try
{
    if (message.empty())
        return;

    if (message[0] == '#') {
        unsigned long cmd_end = message.find(' ');
        if (cmd_end == std::string::npos)
            cmd_end = message.length();
        std::string cmd(message, 1, cmd_end - 1);

        std::map<MessageKind, std::string>::const_iterator it = std::find_if(commands.cbegin(), commands.cend(),
            [&cmd](const std::map<MessageKind, std::string>::value_type& cmd_pair) {
                return cmd == cmd_pair.second;
            });
        if (it == commands.cend()) {
            throw std::runtime_error("unknown command " + cmd);
        }

        MessageKind kind = it->first;
        switch (kind) {
        case MessageKind::subscribe:
        case MessageKind::leave:
        {
            std::string msg(message, cmd_end + 1);
            if (msg.empty())
                throw std::runtime_error("no message value");
            send(Message(0, kind, msg));
            break;
        }
        case MessageKind::who:
            send(Message(0, kind));
            break;
        case MessageKind::close_connection:
            send(Message(0, kind));
            terminate_callback_();
            break;
        case MessageKind::set_echoing:
        case MessageKind::set_processing:
        {
            std::string msg(message, cmd_end + 1);
            if (msg.empty())
                throw std::runtime_error("no message value");
            bool value = msg == "1" || msg == "true" || msg == "enable";
            send(Message(0, kind, value));
            break;
        }
        default:
            throw std::runtime_error("unexpectet message kind");
            break;
        }
    } else {
        Message msg = Message(0, MessageKind::message, message);
        send(Message(0, MessageKind::message, message));
    }
} catch (const std::exception& e) {
    std::cerr << "error while processing input: " << e.what() << std::endl;
}

int Client::prepare_socket(const char* ip_address, unsigned short port)
{
    int clnt_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(clnt_fd < 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while creating socket");

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if(inet_pton(AF_INET, ip_address, &address.sin_addr) <= 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while converting address");


    if(connect(clnt_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while connecting to server");
    return clnt_fd;
}

int Client::read_data()
{
    struct pollfd fds;
    fds.fd = socket_fd_;
    fds.events = POLLIN;

    int poll_res = poll(&fds, 1, timeout_);

    switch (poll_res) {
    case -1:
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while poll socket");
    case 0:
        break;
    default:
        unsigned count = 0;
        count = read(socket_fd_, &data_buff_[0], data_buff_.size());
        if(count > 0)
            return count;
        else {
            std::cout << "socket closed by other side" << std::endl;
            return -1;
        }
    }
    return 0;
}

void Client::message_received(const Message &message)
{
    MessageKind kind = message.kind();
    switch (kind) {
    case MessageKind::server_response:
    {
        std::string output("client id " + std::to_string(message.client_id()) + ": " + std::string(message) + '\n');
        std::cout << output << std::flush;
        break;
    }
    case MessageKind::close_connection:
        terminate_callback_();
        break;
    default:
        throw std::runtime_error("unexpected message kind received");
        break;
    }

}

void Client::send(const Message& message)
{
    raw_data data(message.bytes());
    int count = ::send(socket_fd_, data.data(), data.size(), 0);
    if(count <= 0) {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while sending message");
    }
}
