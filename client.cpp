#include "client.h"

Client::Client(const std::string& ip_address, unsigned short port, unsigned int timeout, receive_callback on_message_receive)
    : socket_fd_(0), timeout_(timeout), data_buff_(DATA_BUFF_LEN), on_receive_(on_message_receive), terminate_(false)
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
            terminate_ = true;
            std::cerr << "error while receiving file: " << e.what() << std::endl;
            break;
        }

        switch (count) {
        case -1:
            terminate_ = true;
        case 0:
            break;
        default:
            try {
                Message msg = Message::from_raw_data(data_buff_, 0);
                on_receive_(msg);
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

void Client::send(const Message& message)
{
    raw_data data(message.bytes());
    int count = ::send(socket_fd_, data.data(), data.size(), 0);
    if(count <= 0) {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while sending message");
    }
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
