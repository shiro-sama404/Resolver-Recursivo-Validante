#pragma once

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <string>
#include <unordered_map>
#include <optional>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <sys/un.h>
#include <chrono>
#include <sstream>
#include <filesystem>

using namespace std;

struct CacheEntry
{
    string value;
    chrono::steady_clock::time_point expiration;
};

class CacheDaemon
{
public:
    CacheDaemon() {};
    void run();
    static void send_command(const string& cmd);

private:
    bool _running = true;
    const string _socketPath = "/tmp/resolver.sock";
    mutex _mtx;
    size_t _maxPositiveSize = 50;
    size_t _maxNegativeSize = 50;
    unordered_map<string, CacheEntry> _positiveCache;
    unordered_map<string, CacheEntry> _negativeCache;

    void handle_client(int client_fd);
    void cleanup();
    bool is_expired(const CacheEntry& entry) const;
    string process_command(const string& cmd);
};