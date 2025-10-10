// dns/dot_cliente.h
#pragma once
#include <string>
#include <vector>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

class DOTCliente {
public:
    DOTCliente(const std::string& servidor, int porta = 853);
    ~DOTCliente();

    bool conectar();
    bool enviarQuery(const std::vector<uint8_t>& query);
    bool receberResposta(std::vector<uint8_t>& resposta);

private:
    std::string server_name;
    int port;

    mbedtls_net_context server_fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
};
