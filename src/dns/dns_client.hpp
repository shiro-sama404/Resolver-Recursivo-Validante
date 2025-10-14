#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include "../server/cache_client.hpp"
#include "dns_mensagem.hpp"

#define MAX_DEPTH 30

class DNSClient
{
public:
    std::vector<uint8_t> resolve(const std::string& domain_name, uint16_t qtype);

private:
    std::vector<uint8_t> execute_query(const std::vector<uint8_t>& query_packet, const std::string& server_ip);
    std::vector<uint8_t> query_udp(const std::vector<uint8_t>& query_packet, const std::string& server_ip, int timeout_sec);
    std::vector<uint8_t> query_tcp(const std::vector<uint8_t>& query_packet, const std::string& server_ip);
    
    enum class ResolutionState {
        ANSWER_FOUND,
        DELEGATION,
        CNAME_REDIRECT,
        CONTINUE,
        ERROR
    };

    ResolutionState process_response(const DNSMensagem& response_message,uint16_t qtype,std::vector<std::string>& current_nameservers,std::string& next_query_name);
                                     
    std::vector<std::string> get_delegated_ns_ips(const DNSMensagem& response_message);
};