#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/x509_crt.h>

#include "dns_mensagem.hpp"

class DOTCliente
{
public:
    bool conectar();     
    bool enviarQuery(DNSMensagem& msg);
    bool receberResposta(DNSMensagem& msg);

    DOTCliente(const std::string& servidor, uint16_t porta = 853);
    ~DOTCliente();
    
private:
    std::string servidor;
    uint16_t porta;

    mbedtls_net_context network_socket;
    mbedtls_ssl_context tls_session;
    mbedtls_ssl_config tls_config;
    mbedtls_entropy_context entropy_source;
    mbedtls_ctr_drbg_context random_generator;
    mbedtls_x509_crt trusted_cert;

    void fecharConexao();
    bool handshakeTLS();
};