#ifndef DNSCLIENT_H
#define DNSCLIENT_H

#include <string>
#include <vector>
#include <cstdint>
#include "dns_mensagem.h"


class DNSClient 
{
    public:

    std::vector<uint8_t> resolvedor(const std::string& nome, uint16_t valor);

    std::vector<uint8_t> udp(const std::vector<uint8_t>& query, const std::string& IPns, uint16_t porta, int tempoTO);
   
};

#endif