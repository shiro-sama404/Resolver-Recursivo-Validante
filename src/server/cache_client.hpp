#pragma once

#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../utils/cache_types.hpp"

/**
 * @brief Fornece métodos estáticos para comunicação com o CacheServer.
 */
class CacheClient
{
public:
    /**
     * @brief Envia um comando de texto para o servidor de cache e retorna a resposta.
     * @param command_str O comando a ser enviado.
     * @return A resposta do servidor como uma string, ou uma string vazia em caso de falha.
     */
    static std::string sendCommand(const std::string& command_str);

    /**
     * @brief Verifica se o servidor de cache está ativo e respondendo a conexões.
     * @param timeout_ms Tempo de espera em milissegundos.
     * @return Verdadeiro se a cache estiver ativa, falso caso contrário.
     */
    static bool isCacheActive(int timeout_ms = 200);
};