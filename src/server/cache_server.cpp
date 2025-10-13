#include "cache_server.hpp"

// --- Métodos Públicos ---

/**
 * @brief Lida com o início, execução e término do servidor.
 */
void CacheServer::run()
{
    check_existing_instance();
    cout << "Iniciando CacheDaemon..." << endl;

    daemonize();

    int server_fd = setup_socket();
    _running = true;

    cout << "CacheDaemon ativo e escutando em " << _socketPath << endl;

    accept_connections(server_fd);

    cleanup(server_fd);
}

string CacheServer::stop()
{
    _running = false;

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock >= 0)
    {
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);
        connect(sock, (sockaddr*)&addr, sizeof(addr));
        close(sock);
    }

    return "[client_side] CacheDaemon encerrado.";
}

// --- Métodos Privados Auxiliares ---

/**
 * @brief Verifica se outro daemon já está rodando ou se há um socket órfão.
 */
void CacheServer::check_existing_instance() const
{
    if (!filesystem::exists(_socketPath))
        return;

    int testSock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (testSock < 0)
        return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(testSock, (sockaddr*)&addr, sizeof(addr)) == 0) {
        cerr << "Erro: o CacheDaemon já está ativo em " << _socketPath << endl;
        close(testSock);
        exit(EXIT_FAILURE);
    } else {
        cerr << "Aviso: socket órfão encontrado. Removendo " << _socketPath << endl;
        unlink(_socketPath.c_str());
    }
    close(testSock);
}

/**
 * @brief Transforma o processo atual em um daemon.
 */
void CacheServer::daemonize() const
{
    pid_t pid = fork();
    if (pid > 0)
        exit(EXIT_SUCCESS);
    else if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (setsid() < 0)
        exit(EXIT_FAILURE);
}

/**
 * @brief Cria, vincula (bind) e escuta (listen) no socket UNIX.
 * @return O descritor de arquivo do socket do servidor.
 */
int CacheServer::setup_socket() const
{
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0)
        exit(EXIT_FAILURE);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0)
    {
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

/**
 * @brief Loop principal que aceita novas conexões de clientes.
 * @param server_fd O descritor de arquivo do servidor.
 */
void CacheServer::accept_connections(int server_fd)
{
    while (_running)
    {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }
        thread(&CacheServer::handle_client, this, client_fd).detach();
    }
}

/**
 * @brief Lida com a comunicação de um único cliente.
 * @param client_fd O descritor de arquivo do cliente conectado.
 */
void CacheServer::handle_client(int client_fd)
{
    char buffer[2048];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);

    if (n > 0)
    {
        buffer[n] = '\0';
        string answer = _controller.processCommand(buffer);

        if (answer == "SHUTDOWN\n")
            answer = CacheServer::stop();

        write(client_fd, answer.c_str(), answer.size());
    }
    close(client_fd);
}

/**
 * @brief Fecha e remove o socket do servidor.
 * @param server_fd O descritor de arquivo do servidor.
 */
void CacheServer::cleanup(int server_fd) const
{
    close(server_fd);
    unlink(_socketPath.c_str());
    cout << "[server_side] CacheDaemon encerrado." << endl;
}