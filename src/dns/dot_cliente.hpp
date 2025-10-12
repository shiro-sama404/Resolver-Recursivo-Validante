#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>

#include "dns_mensagem.hpp"

class DOTCliente {

    DOTCliente(const std::string& servidor, int porta = 853);
    ~DOTCliente();

    bool conectar(int timeout_seg = 5);
    bool enviarQuery(DNSMensagem& msg, int timeout_ms = 5000);
    bool receberResposta(DNSMensagem& msg, int timeout_ms = 5000);

private:

    int porta;
    int socket_fd;
    std::string servidor;
    mbedtls_ssl_context tls_session;
    mbedtls_ssl_config tls_config;
    mbedtls_ctr_drbg_context random_generator;
    mbedtls_entropy_context entropy_source;
    mbedtls_x509_crt trusted_cert;

    void fecharConexao();
    static int enviar_dados(void* ctx_socket, const unsigned char* dados_enviar, size_t tam_dados);
    static int receber_dados(void* ctx, unsigned char* buf, size_t len);
}
