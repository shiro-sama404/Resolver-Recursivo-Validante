#pragma once

#include <optional>
#include <string>

#include "cache_store.hpp"
#include "../utils/cache_types.hpp"

/**
 * @brief Processa objetos de comando, interage com o CacheStore e formata
 * as respostas para o usuário.
 */
class CacheController
{
public:
    explicit CacheController(CacheStore& store)
        : _store(store){};


    /**
     * @brief Ponto de entrada principal para processar qualquer comando.
     * @param cmd O objeto de comando a ser executado.
     * @return Um objeto CacheResponse com a resposta formatada.
     */
    CacheResponse processCommand(const Command& cmd);

private:
    CacheStore& _store;

    CacheResponse handleGetPositive(const Command& cmd);
    CacheResponse handleGetNegative(const Command& cmd);
    CacheResponse handlePutPositive(const Command& cmd);
    CacheResponse handlePutNegative(const Command& cmd);
    CacheResponse handlePurge(const Command& cmd);
    CacheResponse handleSetMaxSize(const Command& cmd);
    CacheResponse handleList(const Command& cmd);
    CacheResponse handleStatus();
};