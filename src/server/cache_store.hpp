#pragma once
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

using namespace std;

enum class CacheTarget
{
    Positive,
    Negative,
    All,
    Invalid
};

enum class NegativeErrorType { NXDOMAIN, NODATA, SERVFAIL, UNKNOWN };

struct CacheEntry
{
    string value;
    int ttl;
    time_t timestamp;
};

class CacheStore
{
public:
    CacheStore() {};

    static string target_to_string(CacheTarget m);
    static CacheTarget string_to_target(const string& s);

    void put(const string& key, const string& value, int ttl, bool positive = true);
    string get(const string& key, bool positive = true);
    void purge(CacheTarget target = CacheTarget::All);
    string list(CacheTarget target = CacheTarget::All);
    void setMaxSize(size_t n, bool positive = true);
    string status() const;

    void startCleanupThread(int intervalSec = 10);
    void stopCleanupThread();

private:
    unordered_map<string, CacheEntry> _positiveCache;
    unordered_map<string, CacheEntry> _negativeCache;
    mutable mutex _mtx;

    size_t _maxPositive = 50;
    size_t _maxNegative = 50;
    bool _running = false;
    thread _cleanupThread;

    void cleanup();
};