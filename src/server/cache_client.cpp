#include "cache_client.hpp"

#include <fcntl.h>

using namespace std;

string CacheClient::sendCommand(const string& command_str)
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        return "";

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/resolver.sock", sizeof(addr.sun_path) - 1);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return "";
    }

    string payload = command_str;
    send(sock, payload.c_str(), payload.size(), 0);

    char buffer[2048];
    ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    string answer;
    if (n > 0)
    {
        buffer[n] = '\0';
        answer = buffer;
    }
    close(sock);
    return answer;
}

bool CacheClient::isCacheActive(int timeout_ms)
{
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0)
        return false;

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/resolver.sock", sizeof(addr.sun_path)-1);

    int result = connect(s, (sockaddr*)&addr, sizeof(addr));
    if (result == 0)
    {
        close(s);
        return true;
    }
    
    if (errno != EINPROGRESS)
    {
        close(s);
        return false;
    }

    fd_set wf;
    FD_ZERO(&wf);
    FD_SET(s, &wf);
    timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    if (select(s + 1, NULL, &wf, NULL, &tv) <= 0)
    {
        close(s);
        return false;
    }

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    
    if (getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &len) != 0)
    {
        close(s);
        return false;
    }

    close(s);
    return (so_error == 0);
}