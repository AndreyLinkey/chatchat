#include <csignal>
#include <thread>
#include <functional>
#include <iostream>
#include <future>
#include <time.h>

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
    struct pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;
    while (!terminate) {
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

int main(int argc, char *argv[])
{
    std::signal(SIGINT, sig_handler);
    std::signal(SIGTERM, sig_handler);

    std::string ip_addr(DEFAULT_IP_ADDR);
    unsigned short port(DEFAULT_PORT);
    std::string group(DEFAULT_GROUP);
    bool echoing(DEFAULT_MESSAGE_ECHOING);
    bool processing(DEFAULT_MESSAGE_PROCESSING);

    int opt;
    while((opt = getopt(argc, argv, "a:p:g:emh")) != -1)
    {
        switch(opt) {
        case 'a':
            ip_addr = std::string(optarg);
            break;
        case 'p':
            try {
                port = static_cast<unsigned short>(std::stoi(std::string(optarg)));
            } catch (...) {
                std::cerr << "invalid value of argument -p: " << optarg << std::endl;
                return 1;
            }
            break;
        case 'g':
            group = std::string(optarg);
            break;
        case 'e':
            echoing = true;
            break;
        case 'm':
            processing = true;
            break;
        case 'h':
            std::cout << "-a server ip address\n-p port number\n-g group name\n-e enable message echoing\n"
                         "-m enable message processing"
                      << std::endl;
            return 0;
        }
    }

    try {
        Client clnt(ip_addr, port, POLL_TIMEOUT, terminate_callback(set_terminate));
        std::thread thr(std::bind(&Client::run, &clnt));
        std::cout << "client started" << std::endl;

        clnt.send_message("#join " + group);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        clnt.send_message(std::string("#echo ") + (echoing ? "1" : "0"));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        clnt.send_message(std::string("#process ") + (processing ? "1" : "0"));

        std::future<std::string> command = std::async(std::launch::async, std::bind(read_command, POLL_TIMEOUT));
        while(!terminate) {
            std::string cmd(command.get());
            command = std::async(std::launch::async, std::bind(read_command, POLL_TIMEOUT));
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
