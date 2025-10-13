#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/time.h> 
#include <unistd.h>
#include <vector>

#include "../server/cache_client.hpp"
#include "dns_mensagem.hpp"

using namespace std;

class DNSClient 
{
public:
    vector<uint8_t> udp(const std::vector<uint8_t>& query, const std::string& IPns, uint16_t porta, int tempoTO);
    vector<uint8_t> resolvedor(const std::string& nome, uint16_t valor);
};