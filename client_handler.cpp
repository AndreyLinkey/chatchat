#include "client_handler.h"

client_handler::client_handler(int client_fd, unsigned timeout, unsigned client_id)
 : client_fd_(client_fd), terminate_(false), timeout_(timeout), client_id_(client_id)
{

}

unsigned client_handler::read_data()
{
    struct pollfd fds;
    fds.fd = client_fd_;
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
        // read data here
        // count = read(client_fd_, buff, buff_size);
        if(count > 0)
            return count;
        else {
            std::cout << "socket closed by other side" << std::endl;
            terminate_ = true;
        }
    }
    return 0;
}

void client_handler::run()
{
    while(!terminate_)
    {
        long count = 0;
        try
        {
            count = read_data();
        }
        catch (const std::exception &e)
        {
            terminate_ = true;
            std::cerr << "error while receiving file: " << e.what() << std::endl;
            break;
        }

        if(count > 0)
        {
            //perfom some action with data
        }
        else
        {
            if(terminate_)
                break;
        }
    }
    close(client_fd_);
}

void client_handler::exit()
{
    terminate_ = true;
}

bool client_handler::terminated()
{
    return terminate_;
}
