#include "cache_controller.hpp"

CacheController::CacheController(CacheStore& store) : _store(store) {}

string CacheController::processCommand(const string& cmd)
{
    istringstream iss(cmd);
    string action; iss >> action;

    if (action == "CACHE_PUT")            return handlePut  (cmd.substr(action.size()), true);
    if (action == "CACHE_PUT_NEGATIVE")   return handlePut  (cmd.substr(action.size()), false);
    if (action == "CACHE_GET")            return handleGet  (cmd.substr(action.size()), true);
    if (action == "CACHE_GET_NEGATIVE")   return handleGet  (cmd.substr(action.size()), false);
    if (action == "PURGE")                return handlePurge(cmd.substr(action.size()));
    if (action == "SET")                  return handleSet  (cmd.substr(action.size()));
    if (action == "LIST")                 return handleList (cmd.substr(action.size()));
    if (action == "STATUS")               return _store.status();
    if (action == "SHUTDOWN")             return "SHUTDOWN\n";
    if (action == "START_CLEANUP_THREAD")
    {
        int expired_purge_interval; iss >> expired_purge_interval;
        cout << "teste" << endl;
        _store.startCleanupThread(expired_purge_interval);
        cout << "teste2" << endl;
        return "Thread de limpeza iniciada a cada " + to_string(expired_purge_interval) + "s.\n";
    }
    if (action == "STOP_CLEANUP_THREAD")   _store.stopCleanupThread (); return "";

    return "Erro: comando desconhecido.\n";
}

string CacheController::handlePut(const string& args, bool positive)
{
    istringstream ss(args);
    string key, value;
    int ttl;
    ss >> key >> value >> ttl;
    if (key.empty() || value.empty() || !ss)
        return "Erro: uso correto: [--cache-put|--cache-put-negative] <key> <value> <ttl>\n";
    _store.put(key, value, ttl, positive);
    return "OK: armazenado " + key + " -> " + value + " (TTL=" + to_string(ttl) + ")\n";
}

string CacheController::handleGet(const string& args, bool positive)
{
    istringstream ss(args);
    string key;
    ss >> key;
    if (key.empty())
        return "Erro: uso correto: --cache-get <nome>\n";
    return _store.get(key, positive);
}

string CacheController::handlePurge(const string& args)
{
    string type;
    istringstream(args) >> type;

    convert_case(type, true);
    CacheTarget target_cache = CacheStore::string_to_target(type);

    if (target_cache == CacheTarget::Invalid)
        return "Erro: uso correto: --purge- POSITIVE|NEGATIVE|ALL\n";

    _store.purge(target_cache);

    if (target_cache == CacheTarget::Positive)
        return "Cache positiva limpa.\n";
    if (target_cache == CacheTarget::Negative)
        return "Cache negativa limpa.\n";
    return "Todas as caches limpas.\n";
}

string CacheController::handleSet(const string& args)
{
    string type;
    size_t size;
    istringstream(args) >> type >> size;

    convert_case(type, false);

    if (type == "POSITIVE")
    {
        _store.setMaxSize(size, true);
        return "Limite positivo ajustado.\n";
    }
    if (type == "NEGATIVE")
    {
        _store.setMaxSize(size, false);
        return "Limite negativo ajustado.\n";
    }
    return "Erro: uso correto: SET POSITIVE|NEGATIVE <n>\n";
}

string CacheController::handleList(const string& args)
{
    string type;
    istringstream(args) >> type;
    convert_case(type, true);
    CacheTarget target_cache = CacheStore::string_to_target(type);

    if (target_cache == CacheTarget::Invalid)
        return "Erro: uso correto: LIST POSITIVE|NEGATIVE|ALL\n";

    return _store.list(target_cache);
}