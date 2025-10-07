#include "cache_daemon.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <cstring>

using namespace std;

CacheDaemon::CacheDaemon() {}

void CacheDaemon::run() {
    int server_fd;
    sockaddr_un addr;

    unlink(_socketPath.c_str());

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    cout << "CacheDaemon ativo e escutando em " << _socketPath << endl;

    while (_running) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        thread(&CacheDaemon::handle_client, this, client_fd).detach();
    }

    close(server_fd);
    unlink(_socketPath.c_str());
}

void CacheDaemon::handle_client(int client_fd) {
    char buffer[1024];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }

    buffer[n] = '\0';
    string cmd(buffer);
    string response = process_command(cmd);
    write(client_fd, response.c_str(), response.size());
    close(client_fd);
}

string CacheDaemon::process_command(const string& cmd) {
    lock_guard<mutex> lock(_mtx);

    if (cmd == "STATUS") {
        return "Cache positiva: " + to_string(_positiveCache.size()) +
               "\nCache negativa: " + to_string(_negativeCache.size()) + "\n";
    }

    if (cmd == "PURGE_ALL") {
        _positiveCache.clear();
        _negativeCache.clear();
        return "Caches expurgadas.\n";
    }

    if (cmd == "DEACTIVATE") {
        _running = false;
        return "Daemon encerrado.\n";
    }

    return "Comando não reconhecido.\n";
}

void CacheDaemon::send_command(const string& cmd) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/resolver.sock", sizeof(addr.sun_path) - 1);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Erro: não foi possível conectar ao CacheDaemon. " << "Talvez ele não esteja ativo." << endl;

        // Remove o socket quebrado se ele existir
        unlink("/tmp/resolver.sock");
        close(sock);
        return;
    }

    // Envia comando ao daemon
    write(sock, cmd.c_str(), cmd.size());

    // Lê resposta do daemon
    char buffer[1024];
    ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        cout << buffer;
    } else {
        cerr << "Aviso: nenhuma resposta recebida do CacheDaemon." << endl;
    }

    close(sock);
}

void CacheDaemon::cleanup() {
    auto now = chrono::steady_clock::now();

    for (auto it = _positiveCache.begin(); it != _positiveCache.end();) {
        if (it->second.expiration <= now) it = _positiveCache.erase(it);
        else ++it;
    }

    for (auto it = _negativeCache.begin(); it != _negativeCache.end();) {
        if (it->second.expiration <= now) it = _negativeCache.erase(it);
        else ++it;
    }
}