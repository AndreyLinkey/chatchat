#include "server.h"

Server::Server(unsigned short port)
    : socket_fd_(0), new_client_idx_(1)
{
    try {
        socket_fd_ = prepare_socket(port);
    }  catch (...) {
        if (socket_fd_) {
            close(socket_fd_);
        }
        throw;
    }

}

void Server::begin_accept(bool &terminate_flag)
{
    while(!terminate_flag) {
        try {
            cleanup_terminated();
            accept_connection();
        } catch (const std::exception &e) {
            std::cerr << "error while accepting connection: " << e.what() << std::endl;
            return;
        }       
    }
    std::lock_guard<std::mutex> lock(processing_);
    for(std::map<unsigned int, client_handler_ptr>::value_type& hdl: clients_) {
        try {
            hdl.second->send(Message(hdl.first, MessageKind::close_connection));
        } catch (std::exception &e) {
            std::cerr << "can't send terminate message to client: " << e.what() << std::endl;
        }
        hdl.second -> exit();
    }
    for(std::map<unsigned int, client_thread_ptr>::value_type& thr: threads_)
        thr.second -> join();

    close(socket_fd_);
}

void Server::message_received(const Message &message)
{
    std::lock_guard<std::mutex> lock(processing_);

    std::map<unsigned int, client_handler_ptr>::const_iterator client_it = clients_.find(message.client_id());
    if (client_it == clients_.cend()) {
        return;
    }

    switch (message.kind()) {
    case MessageKind::message:
    {
        std::vector<unsigned int> handlers(subscribed_handlers(client_it->second->groups()));

        if (!client_it->second->echoing_required()) {
            std::vector<unsigned int>::const_iterator it = std::find(handlers.cbegin(), handlers.cend(), message.client_id());
            if (it != handlers.end())
                handlers.erase(it);
        }

        if (handlers.empty()) {
            break;
        }

        std::vector<unsigned int>::iterator res = std::partition(handlers.begin(), handlers.end(),
            [this](unsigned int id) {
                return this->clients_.at(id)->processing_required();
            }
        );

        if (std::distance(handlers.begin(), res) > 0) {
            std::string processed_message = process_message(message);
            send_message(std::vector<unsigned int>(handlers.begin(), res), message.client_id(), processed_message);
        }
        send_message(std::vector<unsigned int>(res, handlers.end()), message.client_id(), message);
        break;
    }
    case MessageKind::subscribe:
    {
        bool res = client_it->second->subscribe(message);
        if (client_it->second->echoing_required()) {
            send_message(message.client_id(), message.client_id(),
                         res ? "group successfully subscribed" : "group already subscribed");
        }
        break;
    }
    case MessageKind::leave:
    {
        bool res = client_it->second->leave(message);
        if (client_it->second->echoing_required()) {
            send_message(message.client_id(), message.client_id(),
                         res ? "group successfully left" : "unable to leave group");
        }
        break;
    }
    case MessageKind::who:
    {
        std::vector<unsigned int> handlers(subscribed_handlers(client_it->second->groups()));
        std::string res;
        if (handlers.empty()) {
            res = "no members";
        } else {
            for (unsigned int id : handlers) {
                res += std::to_string(id) + (id == message.client_id() ? "(you) " : " ");
            }
            res.pop_back();
        }
        send_message(message.client_id(), message.client_id(), res);
        break;
    }
    case MessageKind::set_echoing:
    {
        client_it->second->set_echoing(message);
        if (client_it->second->echoing_required()) {
            send_message(message.client_id(), message.client_id(),
                         std::string("echoing ") + (message ? "enabled" : "disabled"));
        }
        break;
    }
    case MessageKind::set_processing:
        client_it->second->set_processing(message);
        if (client_it->second->echoing_required()) {
            send_message(message.client_id(), message.client_id(),
                         std::string("processing ") + (message ? "enabled" : "disabled"));
        }
        break;
    case MessageKind::close_connection:
    {
        client_it->second->exit();
        break;
    }
    default:
        throw std::runtime_error("unexpected message kind received");
        break;
    }
}

int Server::prepare_socket(unsigned short port)
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
    return srv_fd;
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

        std::lock_guard<std::mutex> lock(processing_);
        clients_.emplace(std::make_pair(new_client_idx_,
            new client_handler(client_fd, POLL_TIMEOUT, new_client_idx_,
                receive_callback(std::bind(&Server::message_received, this, std::placeholders::_1))
            ))
        );
        threads_.emplace(std::make_pair(new_client_idx_,
            new std::thread(std::bind(&client_handler::run, clients_.at(new_client_idx_).get())))
        );
        std::cout << "client number " << std::to_string(new_client_idx_) << " accepted" << std::endl;
        ++new_client_idx_;
    }
}

void Server::send_message(unsigned int recipient, unsigned int sender, const std::string &message) const
{
    Message msg(sender, MessageKind::server_response, message);
    clients_.at(recipient)->send(msg);
}

void Server::send_message(const std::vector<unsigned int>& recipients, unsigned int sender, const std::string& message) const
{
    for (unsigned int id : recipients) {
        send_message(id, sender, message);
    }
}

void Server::cleanup_terminated()
{
    std::lock_guard<std::mutex> lock(processing_);
    std::map<unsigned int, client_handler_ptr>::iterator it = clients_.begin();
    while(it != clients_.end())
    {
        if((it->second)->terminated())
        {
            unsigned int id = it->first;
            it = clients_.erase(it);
            threads_.at(id)->join();
            threads_.erase(id);
            std::cout << "client number " << std::to_string(id) << " left" << std::endl;
            continue;
        }
        ++it;
    }
}

std::vector<unsigned int> Server::subscribed_handlers(const std::vector<std::string>& groups) const
{
    if (groups.empty())
        return {};

    std::vector<unsigned int> handlers;
    for(const std::map<unsigned int, client_handler_ptr>::value_type& hdl: clients_) {
        for (const std::string& group : groups)
        {
            if (hdl.second->in_group(group)) {
                handlers.push_back(hdl.first);
                break;
            }
        }
    }
    return handlers;
}

std::string Server::process_message(const std::string& message)
{
    // TODO do some transformations with message
    return message;
}
