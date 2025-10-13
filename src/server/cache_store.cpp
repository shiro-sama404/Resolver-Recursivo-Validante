#include "cache_store.hpp"

// Mapeamento
static const unordered_map<string, CacheTarget> target_map = {
    {"positive", CacheTarget::Positive},
    {"negative", CacheTarget::Negative},
    {"all", CacheTarget::All},
    {"invalid", CacheTarget::Invalid}
};

string CacheStore::target_to_string(CacheTarget t)
{
    for (const auto& [key, val] : target_map) 
        if (val == t)
            return key;
    return "invalid";
}

CacheTarget CacheStore::string_to_target(const string& s)
{
    auto it = target_map.find(s);
    if (it != target_map.end())
        return it->second;
    return CacheTarget::Invalid;
}

void CacheStore::put(const string& key, const string& value, int ttl, bool positive)
{
    lock_guard<mutex> lock(_mtx);
    auto& cache = positive ? _positiveCache : _negativeCache;

    if (cache.size() >= (positive ? _maxPositive : _maxNegative))
    {
        if (!cache.empty())
        {
            auto oldest_it = cache.begin();
            
            for (auto it = next(cache.begin()); it != cache.end(); ++it)
                if (it->second.timestamp < oldest_it->second.timestamp)
                    oldest_it = it;
            cache.erase(oldest_it);
        }
    }

    cache[key] = {value, ttl, time(nullptr)};
}

string CacheStore::get(const string& key, bool positive)
{
    lock_guard<mutex> lock(_mtx);
    auto& cache = positive ? _positiveCache : _negativeCache;
    auto it = cache.find(key);
    if (it == cache.end())
        return "MISS\n";

    auto& entry = it->second;
    time_t now = time(nullptr);
    if (difftime(now, entry.timestamp) > entry.ttl)
    {
        cache.erase(it);
        return "MISS (expired)\n";
    }

    return string("HIT ") + entry.value + " TTL=" + to_string(entry.ttl - (int)difftime(now, entry.timestamp)) + "\n";
}

void CacheStore::purge(CacheTarget target)
{
    lock_guard<mutex> lock(_mtx);
    if (target == CacheTarget::Positive || target == CacheTarget::All)
        _positiveCache.clear();
    if (target == CacheTarget::Negative || target == CacheTarget::All)
        _negativeCache.clear();
}

string CacheStore::list(CacheTarget target)
{
    lock_guard<mutex> lock(_mtx);
    string out;

    if (target == CacheTarget::Positive || target == CacheTarget::All)
    {
        out += "---- Cache Positiva ---- (" + to_string(_positiveCache.size()) + " entradas)\n";
        for (const auto& [k, v] : _positiveCache)
        {
            int remaining = max(0, v.ttl - static_cast<int>(difftime(time(nullptr), v.timestamp)));
            out += k + " → " + v.value + " (TTL=" + to_string(remaining) + ")\n";
        }
    }
    if (target == CacheTarget::Negative || target == CacheTarget::All)
    {
        if (!out.empty())
            out += "\n";
        out += "---- Cache Negativa ---- (" + to_string(_negativeCache.size()) + " entradas)\n";
        for (const auto& [k, v] : _negativeCache)
        {
            int remaining = max(0, v.ttl - static_cast<int>(difftime(time(nullptr), v.timestamp)));
            out += k + " → " + v.value + " (TTL=" + to_string(remaining) + ")\n";
        }
    }
    return out.empty() ? "Cache vazia.\n" : out;
}

void CacheStore::setMaxSize(size_t n, bool positive)
{
    if (positive)
        _maxPositive = n;
    else
        _maxNegative = n;
}

string CacheStore::status() const
{
    lock_guard<mutex> lock(_mtx);
    return "Cache positiva: " + to_string(_positiveCache.size()) + "/" + to_string(_maxPositive) + "\n" +
           "Cache negativa: " + to_string(_negativeCache.size()) + "/" + to_string(_maxNegative) + "\n";
}

void CacheStore::startCleanupThread(int intervalSec)
{
    _running = true;
    _cleanupThread = thread([this, intervalSec]() {
        while (_running)
        {
            this->cleanup();
            this_thread::sleep_for(chrono::seconds(intervalSec));
        }
    });
}

void CacheStore::stopCleanupThread()
{
    _running = false;
    if (_cleanupThread.joinable())
        _cleanupThread.join();
}

void CacheStore::cleanup()
{
    lock_guard<mutex> lock(_mtx);
    time_t now = time(nullptr);
    auto pred = [now](auto& p) { return difftime(now, p.second.timestamp) > p.second.ttl; };
    for (auto it = _positiveCache.begin(); it != _positiveCache.end(); )
        it = pred(*it) ? _positiveCache.erase(it) : ++it;
    for (auto it = _negativeCache.begin(); it != _negativeCache.end(); )
        it = pred(*it) ? _negativeCache.erase(it) : ++it;
}