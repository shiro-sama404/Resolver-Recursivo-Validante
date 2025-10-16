#include "cache_store.hpp"

using namespace std;

CacheTarget CacheStore::stringToTarget(const string& s)
{
    if (s == "POSITIVE") return CacheTarget::Positive;
    if (s == "NEGATIVE") return CacheTarget::Negative;
    if (s == "ALL")      return CacheTarget::All;
    return CacheTarget::Invalid;
}

void CacheStore::put(const CacheKey& key, const PositiveCacheEntry& entry)
{
    lock_guard<mutex> lock(_mutex);
    if (_positive_cache.size() >= _max_positive_size && _positive_cache.find(key) == _positive_cache.end())
        if (!_positive_cache.empty())
            _positive_cache.erase(_positive_cache.begin());
    _positive_cache[key] = entry;
}

void CacheStore::put(const CacheKey& key, const NegativeCacheEntry& entry)
{
    lock_guard<mutex> lock(_mutex);
    if (_negative_cache.size() >= _max_negative_size && _negative_cache.find(key) == _negative_cache.end())
        if (!_negative_cache.empty())
            _negative_cache.erase(_negative_cache.begin());
    _negative_cache[key] = entry;
}

optional<PositiveCacheEntry> CacheStore::getPositive(const CacheKey& key)
{
    lock_guard<mutex> lock(_mutex);
    auto it = _positive_cache.find(key);
    if (it == _positive_cache.end())
        return nullopt; // Cache miss

    if (time(nullptr) > it->second.expiration_time)
    {
        _positive_cache.erase(it);
        return nullopt; // Cache miss (expirado)
    }

    return it->second; // Cache hit
}

optional<NegativeCacheEntry> CacheStore::getNegative(const CacheKey& key)
{
    lock_guard<mutex> lock(_mutex);
    auto it = _negative_cache.find(key);
    if (it == _negative_cache.end())
        return nullopt;

    if (time(nullptr) > it->second.expiration_time)
    {
        _negative_cache.erase(it);
        return nullopt;
    }

    return it->second;
}

void CacheStore::purge(CacheTarget target)
{
    lock_guard<mutex> lock(_mutex);
    if (target == CacheTarget::Positive || target == CacheTarget::All)
        _positive_cache.clear();
    if (target == CacheTarget::Negative || target == CacheTarget::All)
        _negative_cache.clear();
}

pair<vector<pair<CacheKey, PositiveCacheEntry>>, vector<pair<CacheKey, NegativeCacheEntry>>> CacheStore::list(CacheTarget target)
{
    lock_guard<mutex> lock(_mutex);
    pair<vector<pair<CacheKey, PositiveCacheEntry>>, vector<pair<CacheKey, NegativeCacheEntry>>> result;
    
    if (target == CacheTarget::Positive || target == CacheTarget::All)
        for(const auto& pair : _positive_cache)
            result.first.push_back(pair);
    if (target == CacheTarget::Negative || target == CacheTarget::All)
        for(const auto& pair : _negative_cache)
            result.second.push_back(pair);
    return result;
}

void CacheStore::setMaxSize(size_t n, bool is_positive)
{
    lock_guard<mutex> lock(_mutex);
    if (is_positive)
        _max_positive_size = n;
    else
        _max_negative_size = n;
}

CacheStatus CacheStore::getStatus() const
{
    lock_guard<mutex> lock(_mutex);
    return {
        _positive_cache.size(),
        _max_positive_size,
        _negative_cache.size(),
        _max_negative_size
    };
}

void CacheStore::startCleanupThread(int interval_sec)
{
    if (!_is_running)
    {
        _is_running = true;
        _cleanup_thread = thread([this, interval_sec]() {
            while (_is_running)
            {
                this_thread::sleep_for(chrono::seconds(interval_sec));
                this->cleanup();
            }
        });
    }
}

void CacheStore::stopCleanupThread()
{
    _is_running = false;
    if (_cleanup_thread.joinable())
        _cleanup_thread.join();
}

void CacheStore::cleanup()
{
    lock_guard<mutex> lock(_mutex);
    const auto now = time(nullptr);
    for (auto it = _positive_cache.begin(); it != _positive_cache.end();)
        if (now > it->second.expiration_time)
            it = _positive_cache.erase(it);
        else
            ++it;
    for (auto it = _negative_cache.begin(); it != _negative_cache.end();)
        if (now > it->second.expiration_time)
            it = _negative_cache.erase(it);
        else
            ++it;
}