#include <csignal>
#include <thread>
#include <functional>
#include <iostream>
#include <future>
#include <map>
#include <algorithm>

#include "client.h"

static std::atomic<bool> terminate;

void set_terminate()
{
    terminate = true;
}

void sig_handler(int signal __attribute__((unused)))
{
    set_terminate();
}

std::string read_command(const unsigned int timeout)
{
    while (!terminate) {
        struct pollfd fds;
        fds.fd = STDIN_FILENO;
        fds.events = POLLIN;

        int poll_res = poll(&fds, 1, timeout);
        if (poll_res <= 0) {
            continue;
        }

        unsigned int count = 0;
        raw_data buff(DATA_BUFF_LEN);
        count = read(STDIN_FILENO, &buff[0], buff.size());
        if(count <= 0) {
            continue;
        }
        std::string command((char*)buff.data(), count);
        command.pop_back();
        return command;
    }

    return {};
}

int main()
{
    std::signal(SIGINT, sig_handler);
    std::signal(SIGTERM, sig_handler);

    try {
        Client clnt(std::string("127.0.0.1"), DEFAULT_PORT, POLL_TIMEOUT, terminate_callback(set_terminate));
        std::thread thr(std::bind(&Client::run, &clnt));
        std::cout << "client started" << std::endl;

        clnt.send_message("#join " + DEFAULT_GROUP);

        while(!terminate) {
            std::future<std::string> command = std::async(std::launch::async, std::bind(read_command, POLL_TIMEOUT));
            std::string cmd = command.get();
            if (cmd.empty()) {
                continue;
            }
            clnt.send_message(cmd);
        }
        clnt.terminate();
        thr.join();
        std::cout << "client terminated" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "client error: " << e.what() << std::endl;
    }

    return 0;
}
