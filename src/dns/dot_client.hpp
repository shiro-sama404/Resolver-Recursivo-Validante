#pragma once

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "dns_message.hpp"

/**
 * @brief Cliente para comunicação DNS over TLS (DoT).
 * @details Encapsula a complexidade da conexão TCP, handshake TLS com mbedtls,
 * e o envio/recebimento de mensagens DNS prefixadas com tamanho.
 */
class DOTClient
{
public:
    /**
     * @brief Constrói um cliente DoT.
     * @param server_hostname O hostname do servidor DoT.
     * @param port A porta do serviço (padrão 853).
     */
    DOTClient(const std::string& server_hostname, int port = 853);

    /**
     * @brief Destrutor. Libera todos os recursos de rede e TLS.
     */
    ~DOTClient();

    /**
     * @brief Estabelece uma conexão TCP e realiza o handshake TLS com o servidor.
     * @param timeout_sec Timeout em segundos para a conexão e o handshake.
     * @return Verdadeiro em caso de sucesso.
     * @throw std::runtime_error em caso de falha.
     */
    bool connect(int timeout_sec = 5);

    /**
     * @brief Envia uma consulta DNS através da sessão TLS segura.
     * @param msg O objeto DNSMessage contendo a query a ser enviada.
     * @param timeout_ms Timeout em milissegundos para a operação de escrita.
     * @return Verdadeiro em caso de sucesso.
     */
    bool sendQuery(DNSMessage& msg, int timeout_ms = 5000);

    /**
     * @brief Recebe uma resposta DNS através da sessão TLS segura.
     * @param msg O objeto DNSMessage que será preenchido com a resposta.
     * @param timeout_ms Timeout em milissegundos para a operação de leitura.
     * @return Verdadeiro em caso de sucesso.
     */
    bool receiveResponse(DNSMessage& msg, int timeout_ms = 5000);

    /**
     * @brief Define o Server Name Indication (SNI) para o handshake TLS.
     * @param sni O hostname a ser usado no SNI.
     */
    void setSni(const std::string& sni) { _sni = sni; }

private:
    int _port;
    int _socket_fd;
    std::string _server_hostname;
    std::string _sni;

    // --- Estruturas mbedtls ---
    mbedtls_ssl_context _tls_session;
    mbedtls_ssl_config _tls_config;
    mbedtls_ctr_drbg_context _random_generator;
    mbedtls_entropy_context _entropy_source;
    mbedtls_x509_crt _trusted_certs;

    void closeConnection();

    static int sendDataCallback(void* ctx_socket, const unsigned char* data, size_t data_len);
    static int receiveDataCallback(void* ctx_socket, unsigned char* buffer, size_t buffer_len);
};