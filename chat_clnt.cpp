#include <csignal>
#include <thread>
#include <functional>
#include <iostream>
#include <future>
#include <map>
#include <algorithm>

#include "client.h"

static std::map<MessageKind, std::string> commands {
    {MessageKind::subscribe, "join"},
    {MessageKind::leave, "leave"},
    {MessageKind::who, "who"},
    {MessageKind::set_echoing, "echo"},
    {MessageKind::set_processing, "process"},
    {MessageKind::close_connection, "close"}
};



static bool terminate = false;

void sig_handler(int signal __attribute__((unused)))
{
    terminate = true;
}

void message_received(const Message &message)
{
    MessageKind kind = message.kind();
    switch (kind) {
    case MessageKind::server_response:
        std::cout << "Message: " << message << std::endl;
        break;
    case MessageKind::close_connection:
        terminate = true;
        break;
    default:
        throw std::runtime_error("unexpected message kind received");
        break;
    }

}

void command_received(Client &clnt, const std::string& command) try
{
    if (command.empty())
        return;

    if (command[0] == '#') {
        unsigned long cmd_end = command.find(' ');
        if (cmd_end == std::string::npos)
            cmd_end = command.length();
        std::string cmd(command, 1, cmd_end - 1);

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
            std::string msg(command, cmd_end);
            if (msg.empty())
                throw std::runtime_error("no message value");
            clnt.send(Message(0, kind, msg));
            break;
        }
        case MessageKind::who:
            clnt.send(Message(0, kind));
        case MessageKind::close_connection:
            clnt.send(Message(0, kind));
            terminate = true;
            break;
        case MessageKind::set_echoing:
        case MessageKind::set_processing:
        {
            std::string msg(command, cmd_end);
            if (msg.empty())
                throw std::runtime_error("no message value");
            bool value = msg == "1" || msg == "true" || msg == "enable";
            clnt.send(Message(0, kind, value));
            break;
        }
        default:
            throw std::runtime_error("unexpectet message kind");
            break;
        }
    } else {
        clnt.send(Message(0, MessageKind::message, command));
    }
} catch (const std::exception& e) {
    std::cerr << "error while processing input: " << e.what() << std::endl;
}


std::string read_command()
{
    std::string command;
    std::cin >> command;
    return command;
}

int main()
{
    std::signal(SIGINT, sig_handler);
    std::signal(SIGTERM, sig_handler);

    try {
        Client clnt(std::string("127.0.0.1"), DEFAULT_PORT, POLL_TIMEOUT, message_received);
        std::thread thr(std::bind(&Client::run, &clnt));

        while(!terminate) {
            std::future<std::string> command = std::async(std::launch::async, read_command);
            std::chrono::milliseconds timeout(500);
            while(!terminate && command.wait_for(timeout) == std::future_status::ready) {
                command_received(clnt, command.get());
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "client error: " << e.what() << std::endl;
    }

    return 0;
}
