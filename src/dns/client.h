#ifndef DNSCLIENT_H
#define DNSCLIENT_H

#include <string>
#include <vector>
#include <cstdint>
#include "dns_mensagem.h"


class DNSClient 
{
public:
    std::vector<uint8_t> resolve(const std::string& name, uint16_t qtype);

    std::vector<uint8_t> send_recv_udp(const std::vector<uint8_t>& query, 
                                       const std::string& ns_ip, 
                                       uint16_t ns_port, 
                                       int timeout_sec);

private:
   
};

#endif