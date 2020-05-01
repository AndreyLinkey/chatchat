#include "server.h"

Server::Server(unsigned short port)
    : socket_fd_(0), client_idx_(1)
{
    try {
        prepare_socket(port);
    }  catch (...) {
        if (socket_fd_) {
            close(socket_fd_);
        }
    }

}

void Server::begin_accept(bool &terminate_flag)
{
    while(!terminate_flag)
    {
        try
        {
            cleanup_terminated();
            accept_connection();
        }
        catch (const std::exception &e)
        {
            std::cerr << "error while accepting connection: " << e.what() << std::endl;
            return;
        }

    }
    for(client_handler_ptr& hdl: clients_)
        hdl -> exit();
    for(client_thread_ptr& thr: threads_)
        thr -> join();

    close(socket_fd_);
}

void Server::prepare_socket(unsigned short port)
{
    int srv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(srv_fd < 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while creating socket");

    int enable = 1;
    if(setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while setting options for socket");

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if(bind(srv_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "error while binding socket");

    if(listen(srv_fd, 3) < 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "unable to listen socket");
}

void Server::accept_connection()
{
    struct pollfd fds;
    fds.fd = socket_fd_;
    fds.events = POLLIN;

    int poll_res = poll(&fds, 1, POLL_TIMEOUT);
    switch (poll_res) {
    case -1:
    case 0:
        return;
    default:
        int client_fd = accept(socket_fd_, nullptr, nullptr);
        if(client_fd < 0)
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "unable to accept connection");

        clients_.emplace_back(new client_handler(client_fd, POLL_TIMEOUT, client_idx_));
        threads_.emplace_back(new std::thread(std::bind(&client_handler::run, clients_.back().get())));
        std::cout << "client number " << std::to_string(client_idx_) << " accepted" << std::endl;
    }
}

void Server::cleanup_terminated()
{
    std::vector<client_handler_ptr>::iterator it = clients_.begin();
    while(it < clients_.end())
    {
        if((*it) -> terminated())
        {
            long idx = std::distance(clients_.begin(), it);
            it = clients_.erase(it);
            threads_[static_cast<unsigned long>(idx)] -> join();
            threads_.erase(threads_.begin() + idx);
            continue;
        }
        ++it;
    }
}

