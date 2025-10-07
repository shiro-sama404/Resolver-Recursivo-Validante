#ifndef CACHE_DAEMON_H
#define CACHE_DAEMON_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <chrono>

using namespace std;

struct CacheEntry {
    string value;
    chrono::steady_clock::time_point expiration;
};

class CacheDaemon {
public:
    CacheDaemon();
    void run();
    static void send_command(const string& cmd);

private:
    void handle_client(int client_fd);
    void cleanup();
    string process_command(const string& cmd);

    unordered_map<string, CacheEntry> _positiveCache;
    unordered_map<string, CacheEntry> _negativeCache;
    mutex _mtx;
    bool _running = true;
    const string _socketPath = "/tmp/resolver.sock";
};

#endif