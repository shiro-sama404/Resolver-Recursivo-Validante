// dns/dot_cliente.cpp
#include "dot_cliente.h"
#include <iostream>
#include <cstring>

DOTCliente::DOTCliente(const std::string& servidor, int porta)
    : server_name(servidor), port(porta) {

    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
}

DOTCliente::~DOTCliente() {
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_net_free(&server_fd);
}

bool DOTCliente::conectar() {
    int ret;
    const char *pers = "dot_client";

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     reinterpret_cast<const unsigned char*>(pers),
                                     strlen(pers))) != 0) {
        std::cerr << "Falha no CTR_DRBG: " << ret << std::endl;
        return false;
    }

    if ((ret = mbedtls_net_connect(&server_fd, server_name.c_str(),
                                   std::to_string(port).c_str(), MBEDTLS_NET_PROTO_TCP)) != 0) {
        std::cerr << "Falha na conexão TCP: " << ret << std::endl;
        return false;
    }

    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        std::cerr << "Falha na configuração SSL: " << ret << std::endl;
        return false;
    }

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        std::cerr << "Falha no setup SSL: " << ret << std::endl;
        return false;
    }

    if ((ret = mbedtls_ssl_set_hostname(&ssl, server_name.c_str())) != 0) {
        std::cerr << "Falha no SNI: " << ret << std::endl;
        return false;
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, nullptr);

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            std::cerr << "Erro no handshake TLS: " << ret << std::endl;
            return false;
        }
    }

    uint32_t flags = mbedtls_ssl_get_verify_result(&ssl);
    if (flags != 0) {
        std::cerr << "Falha na validação do certificado TLS" << std::endl;
        return false;
    }

    return true;
}

bool DOTCliente::enviarQuery(const std::vector<uint8_t>& query) {
    int ret = mbedtls_ssl_write(&ssl, query.data(), query.size());
    if (ret <= 0) {
        std::cerr << "Erro ao enviar query: " << ret << std::endl;
        return false;
    }
    return true;
}

bool DOTCliente::receberResposta(std::vector<uint8_t>& resposta) {
    resposta.clear();
    uint8_t buffer[4096];
    int ret = mbedtls_ssl_read(&ssl, buffer, sizeof(buffer));
    if (ret <= 0) {
        std::cerr << "Erro ao receber resposta: " << ret << std::endl;
        return false;
    }

    resposta.insert(resposta.end(), buffer, buffer + ret);
    return true;
}
