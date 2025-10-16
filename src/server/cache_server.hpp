#pragma once

#include <filesystem>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#include "cache_controller.hpp"
#include "cache_command_parser.hpp"

/**
 * @brief Gerencia o ciclo de vida e a comunicação de rede do servidor de cache (daemon).
 */
class CacheServer
{
public:
    /**
     * @brief Construtor que recebe uma referência para o CacheController.
     * @param controller A instância de CacheController que processará os comandos.
     */
    explicit CacheServer(CacheController& controller)
        : _controller(controller), _is_running(false) {};

    /**
     * @brief Inicia o servidor, transforma-o em daemon e começa a aceitar conexões.
     */
    void run();

    /**
     * @brief Sinaliza para o servidor em execução para que ele desligue.
     */
    void stop();

private:
    void checkExistingInstance() const;
    void daemonize() const;
    int setupSocket() const;
    void acceptConnections(int server_fd);
    void handleClient(int client_fd);
    void cleanup(int server_fd) const;

    CacheController& _controller;
    int _expired_purge_interval = 10; // segundos
    bool _is_running = true;
    const std::string _socket_path = "/tmp/resolver.sock";
};