#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <iostream>
#include <cstring>
#include <sys/time.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

#include "dns_mensagem.hpp"


class DNSClient 
{
public:
    std::vector<uint8_t> resolvedor(const std::string& nome, uint16_t valor);
    std::vector<uint8_t> udp(const std::vector<uint8_t>& query, const std::string& IPns, uint16_t porta, int tempoTO);
};