#include "dns_client.hpp"

vector<uint8_t> DNSClient::resolve(const string& domain_name, uint16_t qtype)
{
    vector<string> nameservers = {"198.41.0.4", "199.9.14.201", "192.33.4.12"};
    
    string current_name_to_query = domain_name;
    int depth = 0;

    while (depth < MAX_DEPTH)
    {
        depth++;

        DNSMensagem query_message;
        query_message.configurarConsulta(current_name_to_query, qtype);
        vector<uint8_t> query_packet = query_message.montarQuery();
        
        vector<uint8_t> response_bytes;
        
        if (nameservers.empty())
            return {};

        response_bytes = execute_query(query_packet, nameservers[0]);

        if (response_bytes.empty())
        {
            nameservers.erase(nameservers.begin());
            continue;
        }

        DNSMensagem response_message;
        response_message.parseResposta(response_bytes);

        ResolutionState state = process_response(response_message, qtype, nameservers, current_name_to_query);

        if (state == ResolutionState::ANSWER_FOUND)
            return response_bytes;
        if (state == ResolutionState::ERROR)
            return response_bytes;
        if (state == ResolutionState::CNAME_REDIRECT)
            depth = 0;
    }

    return {};
}

vector<uint8_t> DNSClient::query_udp(const vector<uint8_t>& query_packet, const string& server_ip, int timeout_sec)
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        return {};

    struct timeval timeout_val;
    timeout_val.tv_sec = timeout_sec;
    timeout_val.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_val, sizeof(timeout_val));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        close(socket_fd);
        return {};
    }

    sendto(socket_fd, query_packet.data(), query_packet.size(), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));

    vector<uint8_t> response_buffer(512);
    socklen_t addr_len = sizeof(server_addr);
    ssize_t bytes_received = recvfrom(socket_fd, response_buffer.data(), response_buffer.size(), 0, (struct sockaddr *)&server_addr, &addr_len);
    
    close(socket_fd);

    if (bytes_received < 0)
        return {};

    response_buffer.resize(bytes_received);
    return response_buffer;
}

vector<uint8_t> DNSClient::query_tcp(const vector<uint8_t>& query_packet, const string& server_ip)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
        return {};

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        close(socket_fd);
        return {};
    }

    uint16_t net_query_size = htons(query_packet.size());
    vector<uint8_t> tcp_packet;
    tcp_packet.push_back(net_query_size >> 8);
    tcp_packet.push_back(net_query_size & 0xFF);
    tcp_packet.insert(tcp_packet.end(), query_packet.begin(), query_packet.end());
    
    send(socket_fd, tcp_packet.data(), tcp_packet.size(), 0);

    vector<uint8_t> size_buffer(2);
    if (recv(socket_fd, size_buffer.data(), 2, MSG_WAITALL) != 2)
    {
        close(socket_fd);
        return {};
    }

    uint16_t response_size = (size_buffer[0] << 8) | size_buffer[1];
    vector<uint8_t> response_buffer(response_size);
    ssize_t total_bytes_received = 0;
    
    while (total_bytes_received < response_size) {
        ssize_t bytes_received_now = recv(socket_fd, response_buffer.data() + total_bytes_received, response_size - total_bytes_received, 0);
        if (bytes_received_now <= 0)
        {
            close(socket_fd);
            return {};
        }
        total_bytes_received += bytes_received_now;
    }

    close(socket_fd);
    return response_buffer;
}

vector<uint8_t> DNSClient::execute_query(const vector<uint8_t>& query_packet, const string& server_ip)
{
    vector<uint8_t> response_bytes = query_udp(query_packet, server_ip, 5);

    if (response_bytes.size() < 4)
        return response_bytes;

    uint16_t flags = (response_bytes[2] << 8) | response_bytes[3];
    bool is_truncated = (flags >> 9) & 1;

    if (is_truncated)
        return query_tcp(query_packet, server_ip);
    return response_bytes;
}

vector<string> DNSClient::get_delegated_ns_ips(const DNSMensagem& response_message)
{
    vector<string> next_server_ips;
    for (const auto& rr_ns : response_message.autoridades)
        if (rr_ns.tipo == 2)
        { 
            string ns_hostname = rr_ns.resposta_parser;
            DNSClient aux_client;
            vector<uint8_t> ns_address_bytes = aux_client.resolve(ns_hostname, 1);
            
            if (!ns_address_bytes.empty())
            {
                DNSMensagem ns_ip_message;
                ns_ip_message.parseResposta(ns_address_bytes);
                if (ns_ip_message.cabecalho.ancount > 0 && ns_ip_message.respostas[0].tipo == 1)
                    next_server_ips.push_back(ns_ip_message.respostas[0].resposta_parser);
            }
        }
    return next_server_ips;
}

DNSClient::ResolutionState DNSClient::process_response
(const DNSMensagem& response_message,uint16_t qtype, vector<string>& current_nameservers, string& next_query_name)
{
    if (response_message.cabecalho.ancount > 0)
        for (const auto& rr : response_message.respostas) {
            if (rr.tipo == qtype)
                return ResolutionState::ANSWER_FOUND;
            if (rr.tipo == 5)
            {
                next_query_name = rr.resposta_parser;
                current_nameservers = {"198.41.0.4", "199.9.14.201", "192.33.4.12"};
                return ResolutionState::CNAME_REDIRECT;
            }
        }

    if (response_message.cabecalho.nscount > 0)
    {
        vector<string> next_ips = get_delegated_ns_ips(response_message);
        if (!next_ips.empty())
        {
            current_nameservers = next_ips;
            return ResolutionState::DELEGATION;
        }
    }
    
    uint16_t rcode = response_message.cabecalho.flags & 0x000F;
    if (rcode == 3 || rcode == 0 && response_message.cabecalho.ancount == 0)
        return ResolutionState::ANSWER_FOUND;

    return ResolutionState::ERROR;
}