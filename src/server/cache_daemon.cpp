#include "cache_daemon.hpp"

using namespace std;

void CacheDaemon::run() {
    if (filesystem::exists(_socketPath))
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock >= 0)
        {
            sockaddr_un addr{};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);
            
            if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0)
            {
                cerr << "Erro: o CacheDaemon já está ativo (socket em uso em "
                     << _socketPath << ")." << endl;
                close(sock);
                exit(EXIT_FAILURE);
            } else
            {
                cerr << "Aviso: socket órfão encontrado. Removendo " << _socketPath << "..." << endl;
                unlink(_socketPath.c_str());
            }
            close(sock);
        }
    }

    cout << "Iniciando Cache Daemon..." << endl;

    // Fork para rodar em background
    pid_t pid = fork();
    if (pid > 0)
        exit(EXIT_SUCCESS);
    else if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    setsid();

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        return;
    }

    cout << "CacheDaemon ativo e escutando em " << _socketPath << endl;

    while (_running)
    {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }
        thread(&CacheDaemon::handle_client, this, client_fd).detach();
    }

    close(server_fd);
    unlink(_socketPath.c_str());
    cout << "CacheDaemon encerrado." << endl;
}

void CacheDaemon::handle_client(int client_fd) {
    char buffer[2048];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0)
    {
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
    cleanup();

    istringstream iss(cmd);
    string action;
    iss >> action;

    // 🔹 STATUS
    if (action == "STATUS") {
        stringstream ss;
        ss << "Cache positiva: " << _positiveCache.size()
           << "/" << _maxPositiveSize << "\n"
           << "Cache negativa: " << _negativeCache.size()
           << "/" << _maxNegativeSize << "\n";
        return ss.str();
    }

    // 🔹 SET POSITIVE / NEGATIVE
    if (action == "SET")
    {
        string type;
        string size_str;
        size_t new_size;
        iss >> type;
        iss >> size_str;

        if (type.empty() || size_str.empty())
            return "Erro: uso correto: SET POSITIVE <n> ou SET NEGATIVE <n>\n";

        try {
            new_size = std::stoul(size_str);
        } catch (const std::exception& e) {
            return string("Erro: valor inválido para tamanho: '") + size_str + "'\n";
        }
        
        if (type == "POSITIVE")
            _maxPositiveSize = new_size;
        else if (type == "NEGATIVE")
            _maxNegativeSize = new_size;
        else
            return "Erro: uso correto: SET POSITIVE <n> ou SET NEGATIVE <n>\n";
        return "Limite ajustado com sucesso.\n";
    }

    // 🔹 PURGE ...
    if (action == "PURGE") {
        string type;
        iss >> type;
        if (type == "ALL" || type.empty()) {
            _positiveCache.clear();
            _negativeCache.clear();
            return "Caches expurgadas.\n";
        }
        if (type == "POSITIVE")
        {
            _positiveCache.clear();
            return "Cache positiva limpa.\n";
        }
        if (type == "NEGATIVE")
        {
            _negativeCache.clear();
            return "Cache negativa limpa.\n";
        }
        return "Erro: uso correto: PURGE [POSITIVE|NEGATIVE|ALL]\n";
    }

    // 🔹 LIST ...
    if (action == "LIST") {
        string type;
        iss >> type;
        stringstream ss;

        auto print_cache = [&](const string& label, const auto& cache) {
            ss << label << " (" << cache.size() << " entradas)\n";
            for (const auto& [key, entry] : cache) {
                auto remaining_ttl = chrono::duration_cast<chrono::seconds>(
                    entry.expiration - chrono::steady_clock::now()
                ).count();
                ss << "  " << key << " -> " << entry.value
                   << " (TTL=" << max(remaining_ttl, 0L) << "s)\n";
            }
        };

        if (type == "POSITIVE")
            print_cache("Cache positiva", _positiveCache);
        else if (type == "NEGATIVE")
            print_cache("Cache negativa", _negativeCache);
        else if (type == "ALL" || type.empty())
        {
            print_cache("Cache positiva", _positiveCache);
            ss << "\n";
            print_cache("Cache negativa", _negativeCache);
        }
        else
            return "Erro: uso correto: LIST [POSITIVE|NEGATIVE|ALL]\n";

        return ss.str();
    }

    // 🔹 CACHE_PUT | CACHE_GET
    if (action == "CACHE")
    {
        string type;
        string name;
        iss >> type;
        iss >> name;
    
        if (type == "PUT") {
            string value;
            int ttl;
            iss >> value >> ttl;
            if (name.empty() || value.empty() || ttl <= 0)
                return "Erro: uso correto: CACHE_PUT <nome> <valor> <ttl>\n";
            if (_positiveCache.size() >= _maxPositiveSize)
                return "Erro: cache positiva cheia.\n";
            _positiveCache[name] = {value, chrono::steady_clock::now() + chrono::seconds(ttl)};
            return "OK: armazenado " + name + " -> " + value + " (TTL=" + to_string(ttl) + ")\n";
        }

        if (type == "GET") {
            if (name.empty()) return "Erro: uso correto: CACHE_GET <nome>\n";
            auto it = _positiveCache.find(name);
            if (it != _positiveCache.end() && !is_expired(it->second))
            {
                auto remaining_ttl = chrono::duration_cast<chrono::seconds>(
                    it->second.expiration - chrono::steady_clock::now()
                ).count();
                return "HIT " + it->second.value + " TTL=" + to_string(remaining_ttl) + "\n";
            }
            return "MISS " + name + "\n";
        }
    }

    if (action == "DEACTIVATE") {
        unlink(_socketPath.c_str());
        _running = false;
        return "Daemon encerrado.\n";
    }

    cout << "Comando recebido: " << cmd << endl;
    return "Comando não reconhecido.\n";
}

void CacheDaemon::cleanup() {
    for (auto it = _positiveCache.begin(); it != _positiveCache.end();)
    {
        if (is_expired(it->second))
            it = _positiveCache.erase(it);
        else ++it;
    }
    for (auto it = _negativeCache.begin(); it != _negativeCache.end();)
    {
        if (is_expired(it->second))
            it = _negativeCache.erase(it);
        else ++it;
    }
}

bool CacheDaemon::is_expired(const CacheEntry& entry) const {
    return chrono::steady_clock::now() > entry.expiration;
}

void CacheDaemon::send_command(const string& cmd) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return;
    }

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/resolver.sock", sizeof(addr.sun_path) - 1);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        cerr << "Erro: não foi possível conectar ao CacheDaemon. Talvez ele não esteja ativo." << endl;
        unlink("/tmp/resolver.sock");
        close(sock);
        return;
    }

    write(sock, cmd.c_str(), cmd.size());
    char buffer[2048];
    ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
    if (n > 0)
    {
        buffer[n] = '\0';
        cout << buffer;
    } else 
        cerr << "Aviso: nenhuma resposta recebida do CacheDaemon." << endl;

    close(sock);

    if (cmd == "DEACTIVATE")
    {
        // Espera até 1s pela remoção do socket
        for (int i = 0; i < 10; ++i) {
            if (access("/tmp/resolver.sock", F_OK) != 0)
                break;
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
}