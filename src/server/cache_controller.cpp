#include "cache_controller.hpp"

#include <algorithm>
#include <sstream>

using namespace std;

CacheResponse CacheController::processCommand(const Command& cmd)
{
    switch (cmd.type)
    {
        case CommandType::PUT_POSITIVE: return handlePutPositive(cmd);
        case CommandType::PUT_NEGATIVE: return handlePutNegative(cmd);
        case CommandType::GET_POSITIVE: return handleGetPositive(cmd);
        case CommandType::GET_NEGATIVE: return handleGetNegative(cmd);
        case CommandType::PURGE:        return handlePurge      (cmd);
        case CommandType::SET_MAX_SIZE: return handleSetMaxSize (cmd);
        case CommandType::LIST:         return handleList       (cmd);
        case CommandType::STATUS:       return handleStatus        ();
        case CommandType::START_CLEANUP_THREAD:
            _store.startCleanupThread(cmd.interval.value_or(10));
            return {"[CACHE] Thread de limpeza iniciada."};
        case CommandType::SHUTDOWN:
            _store.stopCleanupThread();
            return {"[CACHE] Cache Daemon desativado."};
        default:
            return {"[CACHE] Comando desconhecido."};
    }
}

CacheResponse CacheController::handlePutPositive(const Command& cmd)
{
    if (!cmd.key || !cmd.positive_entry)
        return {"[CACHE] Erro: Dados insuficientes para PUT."};
    
    _store.put(*cmd.key, *cmd.positive_entry);
    return {"[CACHE] OK: Entrada positiva armazenada para " + cmd.key->qname};
}

CacheResponse CacheController::handlePutNegative(const Command& cmd)
{
    if (!cmd.key || !cmd.negative_entry)
    {
        return {"[CACHE] Erro: Dados insuficientes para PUT."};
    }

    _store.put(*cmd.key, *cmd.negative_entry);
    return {"[CACHE] OK: Entrada negativa armazenada para " + cmd.key->qname};
}

CacheResponse CacheController::handleGetPositive(const Command& cmd)
{
    if (!cmd.key)
        return {"[CACHE] Erro: Chave não fornecida para GET."};

    auto result = _store.getPositive(*cmd.key);
    if (result)
    {
        string data_str = visit([](auto&& arg) -> string {
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, string>)
                return arg;
            else
                return "[CACHE] dados binarios";
        }, result->rdata);

        return {"[CACHE HIT] " + data_str, result};
    }
    return {"[CACHE MISS]"};
}

CacheResponse CacheController::handleGetNegative(const Command& cmd)
{
    if (!cmd.key)
        return {"[CACHE] Erro: Chave não fornecida para GET."};

    auto result = _store.getNegative(*cmd.key);
    if (result)
    {
        string reason_str = (result->reason == NegativeReason::NXDOMAIN) ? "NXDOMAIN" : "NODATA";
        return {"[CACHE HIT] " + reason_str, nullopt, result};
    }
    return {"[CACHE MISS]"};
}

CacheResponse CacheController::handlePurge(const Command& cmd)
{
    if (!cmd.target)
        return {"[CACHE] Erro: Alvo de purge não especificado."};
    
    _store.purge(*cmd.target);
    return {"[CACHE] Cache limpa com sucesso."};
}

CacheResponse CacheController::handleSetMaxSize(const Command& cmd)
{
    if (!cmd.size || !cmd.is_positive)
        return {"[CACHE] Erro: Parâmetros inválidos para SET."};
    
    _store.setMaxSize(*cmd.size, *cmd.is_positive);
    return {"[CACHE] Tamanho máximo da cache atualizado."};
}

CacheResponse CacheController::handleList(const Command& cmd)
{
    if (!cmd.target)
        return {"[CACHE] Erro: Alvo de list não especificado."};

    auto lists = _store.list(*cmd.target);
    stringstream ss;

    if (*cmd.target == CacheTarget::Positive || *cmd.target == CacheTarget::All)
    {
        ss << "==== Cache Positiva (" << lists.first.size() << " entradas) ====\n";
        for (const auto& [key, entry] : lists.first)
        {
            long remaining_ttl = max(0L, entry.expiration_time - time(nullptr));
            ss << key.qname << " (T:" << key.qtype << ") -> ... (TTL: " << remaining_ttl << "s)\n";
        }
    }
    if (*cmd.target == CacheTarget::Negative || *cmd.target == CacheTarget::All)
    {
        if (ss.tellp() > 0) ss << "\n";
        ss << "==== Cache Negativa (" << lists.second.size() << " entradas) ====\n";
        for (const auto& [key, entry] : lists.second)
        {
            long remaining_ttl = max(0L, entry.expiration_time - time(nullptr));
            string reason = entry.reason == NegativeReason::NXDOMAIN ? "NXDOMAIN" : "NODATA";
            ss << key.qname << " (T:" << key.qtype << ") -> " << reason << " (TTL: " << remaining_ttl << "s)\n";
        }
    }
    return {ss.str()};
}

CacheResponse CacheController::handleStatus()
{
    auto status = _store.getStatus();
    stringstream ss;
    ss << "[Cache Positiva]: " << status.positive_current_size << "/" << status.positive_max_size << "\n";
    ss << "[Cache Negativa]: " << status.negative_current_size << "/" << status.negative_max_size << "\n";
    return {ss.str()};
}