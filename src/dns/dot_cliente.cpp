#include "dot_cliente.h"
#include <iostream>
#include <cstring>

using namespace std;

DOTCliente::DOTCliente(const string& servidor, uint16_t porta) : servidor(servidor), porta(porta) {
    mbedtls_net_init(&network_socket);
    mbedtls_ssl_init(&tls_session);
    mbedtls_ssl_config_init(&tls_config);
    mbedtls_entropy_init(&entropy_source);
    mbedtls_ctr_drbg_init(&random_generator);
    mbedtls_x509_crt_init(&trusted_cert);
}

void DOTCliente::fecharConexao() {
    mbedtls_ssl_close_notify(&tls_session);  
    mbedtls_net_free(&network_socket);      
}

DOTCliente::~DOTCliente() {
    fecharConexao();
    mbedtls_x509_crt_free(&trusted_cert);
    mbedtls_ssl_free(&tls_session);
    mbedtls_ssl_config_free(&tls_config);
    mbedtls_ctr_drbg_free(&random_generator);
    mbedtls_entropy_free(&entropy_source);
    mbedtls_net_free(&network_socket);
}

bool DOTCliente::handshakeTLS() {

    // inicia a conexão com tls lado cliente
    if (mbedtls_ssl_config_defaults(&tls_config,
            MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM,
            MBEDTLS_SSL_PRESET_DEFAULT) != 0)
    {
        throw runtime_error("Erro ao configurar SSL");
    }

    // manda fazer verificações de segurança
    mbedtls_ssl_conf_authmode(&tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&tls_config, &trusted_cert, nullptr);
    mbedtls_ssl_conf_rng(&tls_config, mbedtls_ctr_drbg_random, &random_generator);

    // estabelece conexão segura
    if (mbedtls_ssl_setup(&tls_session, &tls_config) != 0) {
        throw runtime_error("Erro ao inicializar contexto SSL");
    }

    // define sni - qual servidor quero acessar
    if (mbedtls_ssl_set_hostname(&tls_session, servidor.c_str()) != 0) {
        throw runtime_error("Erro ao configurar SNI");
    }

    mbedtls_ssl_set_bio(&tls_session, &network_socket, mbedtls_net_send, mbedtls_net_recv, nullptr);

    int status;

    // handshake
    while ((status = mbedtls_ssl_handshake(&tls_session)) != 0) {
        if (status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
            char buf[100];
            mbedtls_strerror(status, buf, sizeof(buf));
            throw runtime_error(string("Erro no handshake TLS: ") + buf);
        }
    }


    // valida certificado
    uint32_t flags = mbedtls_ssl_get_verify_result(&tls_session);
    if (flags != 0) {
        char msg_erro[512];
        mbedtls_x509_crt_verify_info(msg_erro, sizeof(msg_erro), "", flags);
        throw runtime_error(string("Falha na validação do certificado: ") + msg_erro);
    }


    return true;
}



bool DOTCliente::conectar() {
    const char* personalisar = "dot_client";

    // inicializa random number generator
    if (mbedtls_ctr_drbg_seed(&random_generator, mbedtls_entropy_func, &entropy_source, reinterpret_cast<const unsigned char*>(personalisar),
                              strlen(personalisar)) != 0)
    {
        throw runtime_error("Erro ao inicializar RNG"); // random number generator
    }

    // carrega certificados confiáveis
    if (mbedtls_x509_crt_parse_path(&trusted_cert, "/etc/ssl/certs") != 0) {
        throw runtime_error("Erro ao carregar certificados CA");
    }

    // conecta tcp ao dot
    if (mbedtls_net_connect(&network_socket, servidor.c_str(), to_string(porta).c_str(), MBEDTLS_NET_PROTO_TCP) != 0)
    {
        throw runtime_error("Erro ao conectar ao servidor " + servidor);
    }

    if (!handshakeTLS()) { 
        fecharConexao();
        throw runtime_error("Falha no handshake TLS com " + servidor);
    }

    return true;
}




