#include "cache_client.hpp"

string CacheClient::send_command(const string& cmd)
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return "";
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/resolver.sock", sizeof(addr.sun_path) - 1);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return "";
    }

    string payload = cmd + "\n";
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

bool CacheClient::is_cache_active(int timeout_ms) {
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0)
        return false;

    // configura non-blocking
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/resolver.sock", sizeof(addr.sun_path)-1);

    int res = connect(s, (sockaddr*)&addr, sizeof(addr));
    if (res == 0)
    {
        close(s);
        return true;
    }
    if (errno != EINPROGRESS)
    {
        close(s);
        return false;
    }

    // espera socket ficar writeable
    fd_set wf;
    FD_ZERO(&wf);
    FD_SET(s, &wf);
    timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int sel = select(s+1, NULL, &wf, NULL, &tv);
    if (sel <= 0) // timeout ou erro
    {
        close(s);
        return false;
    } 

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &len);
    close(s);
    return (so_error == 0);
}