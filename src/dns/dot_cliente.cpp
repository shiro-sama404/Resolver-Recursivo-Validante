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





