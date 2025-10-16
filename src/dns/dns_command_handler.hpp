#pragma once

#include <string>
#include <vector>

#include "../cli/arguments.hpp"
#include "../server/cache_client.hpp"
#include "../utils/cache_types.hpp"
#include "dns_client.hpp"
#include "dns_message.hpp"
#include "../utils/stringUtils.hpp"

/**
 * @brief Orquestra a execução de uma consulta DNS a partir de argumentos da linha de comando.
 * @details Valida os argumentos, gerencia a interação com o cache e coordena
 * a chamada ao resolvedor de rede (DNSClient).
 */
class DNSCommandHandler
{
public:
    /**
     * @brief Construtor que recebe referências para os argumentos e o cliente DNS.
     * @param args Os argumentos da linha de comando.
     * @param client O cliente DNS a ser usado para a resolução de rede.
     */
    DNSCommandHandler(const Arguments& args, DNSClient& client)
        : _args(args), _client(client){};

    /**
     * @brief Ponto de entrada que executa todo o fluxo de uma consulta DNS.
     * @return EXIT_SUCCESS ou EXIT_FAILURE.
     */
    int execute();

private:
    bool validateArgs();
    void configureClient(Mode mode);
    void processResponse(const std::vector<uint8_t>& response_bytes);

    const Arguments& _args;
    DNSClient& _client;
};