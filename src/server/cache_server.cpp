#include "cache_server.hpp"

#include <csignal>
#include <iostream>

using namespace std;

void CacheServer::run()
{
    checkExistingInstance();
    cout << "Iniciando Cache Daemon..." << endl;

    daemonize();

    int server_fd = setupSocket();
    _is_running = true;

    Command start_cleanup_cmd = { CommandType::START_CLEANUP_THREAD, nullopt, nullopt, nullopt, nullopt, nullopt, nullopt, _expired_purge_interval };
    cout << _controller.processCommand(start_cleanup_cmd).message << endl;

    acceptConnections(server_fd);

    cleanup(server_fd);
}

void CacheServer::stop()
{
    _is_running = false;

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock >= 0)
    {
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, _socket_path.c_str(), sizeof(addr.sun_path) - 1);
        connect(sock, (sockaddr*)&addr, sizeof(addr));
        close(sock);
    }
}

void CacheServer::checkExistingInstance() const
{
    if (!filesystem::exists(_socket_path))
    {
        return;
    }

    int test_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (test_sock < 0)
        return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, _socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(test_sock, (sockaddr*)&addr, sizeof(addr)) == 0)
    {
        cerr << "Erro: o Cache Daemon já está ativo." << endl;
        close(test_sock);
        exit(EXIT_FAILURE);
    }
    else
        unlink(_socket_path.c_str());
    close(test_sock);
}

void CacheServer::daemonize() const
{
    pid_t pid = fork();
    if (pid > 0) exit(EXIT_SUCCESS);
    if (pid < 0) exit(EXIT_FAILURE);

    if (setsid() < 0) exit(EXIT_FAILURE);
}

int CacheServer::setupSocket() const
{
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) exit(EXIT_FAILURE);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, _socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0)
    {
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

void CacheServer::acceptConnections(int server_fd)
{
    while (_is_running)
    {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0)
            continue;
        thread(&CacheServer::handleClient, this, client_fd).detach();
    }
}

void CacheServer::handleClient(int client_fd)
{
    char buffer[2048];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    string response_message;

    if (n > 0)
    {
        buffer[n] = '\0';
        try
        {
            Command command = CommandParser::parse(buffer);
            CacheResponse response = _controller.processCommand(command);
            response_message = response.message;

            if (command.type == CommandType::SHUTDOWN)
                stop();
        }
        catch (const exception& e)
        {
            response_message = "Erro: " + string(e.what()) + "\n";
        }

        write(client_fd, response_message.c_str(), response_message.size());
    }
    close(client_fd);
}

void CacheServer::cleanup(int server_fd) const
{
    close(server_fd);
    unlink(_socket_path.c_str());
}