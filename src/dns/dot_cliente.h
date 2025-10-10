#ifndef DOT_CLIENTE_H
#define DOT_CLIENTE_H

#include "dns_mensagem.h"
#include <string>
#include <vector>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/x509_crt.h>


class DOTCliente {
public:

    DOTCliente(const std::string& servidor, uint16_t porta = 853);
    ~DOTCliente();

private:
    std::string servidor;
    uint16_t porta;

};

#endif 