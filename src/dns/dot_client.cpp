#include "dot_client.hpp"

using namespace std;

DOTClient::DOTClient(const string& server_hostname, int port)
    : _port(port), _socket_fd(-1), _server_hostname(server_hostname), _sni(server_hostname)
{
    mbedtls_ssl_init(&_tls_session);
    mbedtls_ssl_config_init(&_tls_config);
    mbedtls_ctr_drbg_init(&_random_generator);
    mbedtls_entropy_init(&_entropy_source);
    mbedtls_x509_crt_init(&_trusted_certs);
}

DOTClient::~DOTClient()
{
    closeConnection();

    mbedtls_ssl_free(&_tls_session);
    mbedtls_ssl_config_free(&_tls_config);
    mbedtls_ctr_drbg_free(&_random_generator);
    mbedtls_entropy_free(&_entropy_source);
    mbedtls_x509_crt_free(&_trusted_certs);
}

bool DOTClient::connect(int timeout_sec)
{
    // Carrega certificados de CA
    int cert_load_result = mbedtls_x509_crt_parse_file(&_trusted_certs, "dns/cacert.pem");
    if (cert_load_result != 0)
        throw runtime_error("Erro ao carregar cacert.pem: " + to_string(cert_load_result));

    // Inicializa o gerador RNG
    const char* personalization = "dot_client";
    int result = mbedtls_ctr_drbg_seed(&_random_generator, mbedtls_entropy_func, &_entropy_source,
        reinterpret_cast<const unsigned char*>(personalization), strlen(personalization));
    if (result != 0)
        throw runtime_error("Erro ao inicializar RNG: " + to_string(result));

    struct addrinfo hints {};
    struct addrinfo* network_result;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(_server_hostname.c_str(), to_string(_port).c_str(), &hints, &network_result) != 0)
        throw runtime_error("Falha ao resolver endereço do servidor " + _server_hostname);

    _socket_fd = socket(network_result->ai_family, network_result->ai_socktype, network_result->ai_protocol);
    if (_socket_fd < 0)
    {
        freeaddrinfo(network_result);
        throw runtime_error("Falha ao criar socket");
    }

    int flags = fcntl(_socket_fd, F_GETFL, 0);
    fcntl(_socket_fd, F_SETFL, flags | O_NONBLOCK);
    result = ::connect(_socket_fd, network_result->ai_addr, network_result->ai_addrlen);

    if (result < 0 && errno != EINPROGRESS)
    {
        freeaddrinfo(network_result);
        throw runtime_error("Erro ao conectar socket");
    }

    if (result < 0)
    {
        fd_set write_sockets;
        FD_ZERO(&write_sockets);
        FD_SET(_socket_fd, &write_sockets);
        struct timeval timeout = {timeout_sec, 0};

        result = select(_socket_fd + 1, nullptr, &write_sockets, nullptr, &timeout);
        if (result <= 0)
        {
            freeaddrinfo(network_result);
            closeConnection();
            throw runtime_error("Timeout ao conectar ao servidor " + _server_hostname);
        }
    }
    
    fcntl(_socket_fd, F_SETFL, flags); // Restaura para modo bloqueante para mbedtls
    freeaddrinfo(network_result);

    // Configuração e realização do handshake TLS
    mbedtls_ssl_config_defaults(&_tls_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_authmode(&_tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&_tls_config, mbedtls_ctr_drbg_random, &_random_generator);
    mbedtls_ssl_conf_ca_chain(&_tls_config, &_trusted_certs, nullptr);

    if ((result = mbedtls_ssl_setup(&_tls_session, &_tls_config)) != 0)
        throw runtime_error("Falha ao configurar SSL: " + to_string(result));
    if ((result = mbedtls_ssl_set_hostname(&_tls_session, _sni.c_str())) != 0)
        throw runtime_error("Falha ao configurar SNI: " + to_string(result));

    mbedtls_ssl_set_bio(&_tls_session, &_socket_fd, sendDataCallback, receiveDataCallback, nullptr);

    while ((result = mbedtls_ssl_handshake(&_tls_session)) != 0)
        if (result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE)
            throw runtime_error("Falha no handshake TLS: " + to_string(result));

    // Verifica certificados do servidor
    uint32_t verification_result = mbedtls_ssl_get_verify_result(&_tls_session);
    if (verification_result != 0)
    {
        char error_buffer[512];
        mbedtls_x509_crt_verify_info(error_buffer, sizeof(error_buffer), "", verification_result);
        throw runtime_error(string("Falha na verificação do certificado: ") + error_buffer);
    }

    return true;
}

bool DOTClient::sendQuery(DNSMessage& msg, int timeout_ms)
{
    vector<uint8_t> packet_to_send = msg.buildQuery();
    uint16_t packet_size = packet_to_send.size();
    
    vector<uint8_t> send_buffer;
    send_buffer.push_back(packet_size >> 8);
    send_buffer.push_back(packet_size & 0xFF);
    send_buffer.insert(send_buffer.end(), packet_to_send.begin(), packet_to_send.end());

    size_t total_bytes_sent = 0;
    while (total_bytes_sent < send_buffer.size())
    {
        int bytes_sent = mbedtls_ssl_write(&_tls_session, send_buffer.data() + total_bytes_sent, send_buffer.size() - total_bytes_sent);
        if (bytes_sent > 0)
            total_bytes_sent += bytes_sent;
        else if (bytes_sent != MBEDTLS_ERR_SSL_WANT_READ && bytes_sent != MBEDTLS_ERR_SSL_WANT_WRITE)
            return false; // Erro de escrita
    }
    return true;
}

bool DOTClient::receiveResponse(DNSMessage& msg, int timeout_ms)
{
    vector<uint8_t> receive_buffer(2);
    size_t total_bytes_received = 0;

    mbedtls_ssl_conf_read_timeout(&_tls_config, timeout_ms);

    while (total_bytes_received < 2)
    {
        int bytes_received = mbedtls_ssl_read(&_tls_session, receive_buffer.data() + total_bytes_received, 2 - total_bytes_received);
        if (bytes_received > 0)
            total_bytes_received += bytes_received;
        else if (bytes_received != MBEDTLS_ERR_SSL_WANT_READ && bytes_received != MBEDTLS_ERR_SSL_WANT_WRITE)
            return false;
    }

    uint16_t packet_size = (receive_buffer[0] << 8) | receive_buffer[1];
    if (packet_size == 0) return false;

    vector<uint8_t> final_packet(packet_size);
    total_bytes_received = 0;

    while (total_bytes_received < packet_size)
    {
        int bytes_received = mbedtls_ssl_read(&_tls_session, final_packet.data() + total_bytes_received, packet_size - total_bytes_received);
        if (bytes_received > 0)
            total_bytes_received += bytes_received;
        else if (bytes_received != MBEDTLS_ERR_SSL_WANT_READ && bytes_received != MBEDTLS_ERR_SSL_WANT_WRITE)
            return false;
    }

    msg.parseResponse(final_packet);
    return true;
}

void DOTClient::closeConnection()
{
    if (_socket_fd != -1)
    {
        mbedtls_ssl_close_notify(&_tls_session);
        close(_socket_fd);
        _socket_fd = -1;
    }
}

int DOTClient::sendDataCallback(void* ctx_socket, const unsigned char* data_to_send, size_t data_len)
{
    int socket_fd = *static_cast<int*>(ctx_socket);
    return send(socket_fd, data_to_send, data_len, 0);
}

int DOTClient::receiveDataCallback(void* ctx_socket, unsigned char* buffer, size_t buffer_len)
{
    int socket_fd = *static_cast<int*>(ctx_socket);
    return recv(socket_fd, buffer, buffer_len, 0);
}