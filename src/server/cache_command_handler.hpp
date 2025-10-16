#pragma once

#include <string>
#include <vector>

#include "../cli/arguments.hpp"
#include "cache_server.hpp"

/**
 * @brief Classe estática responsável por executar comandos de cache a partir
 * da linha de comando.
 */
class CacheCommandHandler
{
public:
    /**
     * @brief Ponto de entrada para lidar com todos os subcomandos de cache.
     * @details Analisa os argumentos, constrói a string de comando apropriada,
     * envia para o servidor via CacheClient e imprime a resposta.
     * @param args O objeto de argumentos já analisado.
     * @param argc A contagem de argumentos da linha de comando.
     * @param argv O vetor de argumentos da linha de comando.
     * @return EXIT_SUCCESS ou EXIT_FAILURE.
     */
    static int execute(const Arguments& args, int argc, char* argv[]);
};