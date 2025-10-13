#pragma once
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std;

class CacheClient
{
public:
    static string send_command(const string& cmd);
    static bool is_cache_active(int timeout_ms = 200);
};
