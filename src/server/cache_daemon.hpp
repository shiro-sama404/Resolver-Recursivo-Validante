#pragma once

#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>
#include <optional>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <sys/un.h>
#include <chrono>

using namespace std;

struct CacheEntry
{
    string value;
    chrono::steady_clock::time_point expiration;
};

class CacheDaemon
{
public:
    CacheDaemon();
    void run();
    static void send_command(const string& cmd);

private:
    unordered_map<string, CacheEntry> _positiveCache;
    unordered_map<string, CacheEntry> _negativeCache;
    mutex _mtx;
    bool _running = true;
    const string _socketPath = "/tmp/resolver.sock";

    void handle_client(int client_fd);
    void cleanup();
    string process_command(const string& cmd);
};