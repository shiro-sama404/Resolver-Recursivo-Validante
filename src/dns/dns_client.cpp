#include "dns_client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

using namespace std;

vector<uint8_t> DNSClient::resolve(const string& domain_name, uint16_t qtype)
{
    vector<string> nameservers;
    if (!_forwarder_ip.empty())
        nameservers.push_back(_forwarder_ip);
    else
        // Servidores raiz (Root Hints)
        nameservers = {"198.41.0.4", "199.9.14.201", "192.33.4.12"};

     if (!_is_recursive)
    {
        DNSMessage query_message;
        query_message.configureQuery(domain_name, qtype);
        query_message.header.flags &= ~0x0100; // Garante bit RD = 0

        vector<uint8_t> query_packet = query_message.buildQuery();
        if (nameservers.empty())
            return {};
        return queryUdp(query_packet, nameservers[0]);
    }

    string current_name_to_query = domain_name;
    int depth = 0;

    while (depth < MAX_DEPTH)
    {
        depth++;

        DNSMessage query_message;
        query_message.configureQuery(current_name_to_query, qtype);
        if (_is_recursive)
            query_message.header.flags |= 0x0100; // Seta o bit RD
        else
            query_message.header.flags &= ~0x0100; // Limpa o bit RD
        
        vector<uint8_t> query_packet = query_message.buildQuery();

        if (nameservers.empty())
            return {};

        string current_ns = nameservers[0];
        vector<uint8_t> response_bytes;

        if (_use_dot)
        {
            try
            {
                response_bytes = queryDot(query_packet, current_ns);
            }
            catch (const exception& e)
            {
                cerr << "[DoT] Erro: " << e.what() << ". Tentando UDP/TCP..." << endl;
            }
        }

        if (response_bytes.empty())
            response_bytes = queryUdp(query_packet, current_ns);

        // Tenta TCP se UDP falhar ou a resposta vier truncada (TC=1)
        DNSMessage temp_response;
        bool is_truncated = false;
        if (!response_bytes.empty())
        {
            try
            {
                temp_response.parseResponse(response_bytes);
                if (temp_response.header.flags & 0x0200) // TC bit
                    is_truncated = true;
            }
            catch(...) { /* Ignora falha de parse aqui */ }
        }

        if (response_bytes.empty() || is_truncated)
            response_bytes = queryTcp(query_packet, current_ns);
        
        if (response_bytes.empty())
        {
            nameservers.erase(nameservers.begin()); // Falha com este NS, tenta o próximo
            continue;
        }

        DNSMessage response;
        try
        {
            response.parseResponse(response_bytes);
        }
        catch (const exception& e)
        {
            nameservers.erase(nameservers.begin()); // Resposta inválida, tenta o próximo NS
            continue;
        }

        ResolutionState state = processResponse(response, nameservers, current_name_to_query);

        if (state == ResolutionState::AnswerFound || state == ResolutionState::Error)
            return response_bytes; // Retorna a resposta  para o handler

        if (state == ResolutionState::CnameRedirect)
            depth = 0; // Reinicia a busca para o novo CNAME
    }

    return {}; // Atingiu profundidade máxima
}

vector<uint8_t> DNSClient::queryUdp(const vector<uint8_t>& query_packet, const string& server_ip, int timeout_sec)
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        return {};

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(53);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) <= 0)
    {
        close(socket_fd);
        return {};
    }

    // Configura o timeout de recebimento no socket
    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t bytes_sent = sendto(socket_fd, query_packet.data(), query_packet.size(), 0,
        (sockaddr*)&server_address, sizeof(server_address));
    if (bytes_sent < 0)
    {
        close(socket_fd);
        return {};
    }

    vector<uint8_t> receive_buffer(4096);
    ssize_t bytes_received = recvfrom(socket_fd, receive_buffer.data(), receive_buffer.size(), 0, nullptr, nullptr);
    if (bytes_received <= 0)
    {
        close(socket_fd);
        return {};
    }

    receive_buffer.resize(bytes_received);
    close(socket_fd);
    return receive_buffer;
}

vector<uint8_t> DNSClient::queryTcp(const vector<uint8_t>& query_packet, const string& server_ip, int timeout_sec)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
        return {};

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(53);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) <= 0)
    {
        close(socket_fd);
        return {};
    }

    // Conexão com timeout
    int socket_flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, socket_flags | O_NONBLOCK);
    int result = connect(socket_fd, (sockaddr*)&server_address, sizeof(server_address));

    if (result < 0 && errno != EINPROGRESS)
    {
        close(socket_fd);
        return {};
    }

    if (result < 0)
    {
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(socket_fd, &write_fds);
        struct timeval timeout = {timeout_sec, 0};
        if (select(socket_fd + 1, nullptr, &write_fds, nullptr, &timeout) <= 0)
        {
            close(socket_fd);
            return {};
        }
    }
    
    fcntl(socket_fd, F_SETFL, socket_flags); // Restaurar para modo bloqueante

    // Envia o pacote com prefixo de tamanho de 2 bytes
    uint16_t length_prefix = htons(static_cast<uint16_t>(query_packet.size()));
    if (send(socket_fd, &length_prefix, 2, 0) != 2)
    {
        close(socket_fd);
        return {};
    }
    if (send(socket_fd, query_packet.data(), query_packet.size(), 0) != (ssize_t)query_packet.size())
    {
        close(socket_fd);
        return {};
    }

    // Recebe a resposta com prefixo de tamanho
    uint8_t size_buffer[2];
    if (recv(socket_fd, size_buffer, 2, MSG_WAITALL) != 2)
    {
        close(socket_fd);
        return {};
    }
    
    uint16_t response_size = (size_buffer[0] << 8) | size_buffer[1];
    if (response_size == 0)
    {
        close(socket_fd);
        return {};
    }

    vector<uint8_t> response_buffer(response_size);
    ssize_t total_bytes_received = 0;
    while (total_bytes_received < response_size)
    {
        ssize_t bytes_received = recv(socket_fd, response_buffer.data() + total_bytes_received, response_size - total_bytes_received, 0);
        if (bytes_received <= 0)
        {
            close(socket_fd);
            return {};
        }
        total_bytes_received += bytes_received;
    }

    close(socket_fd);
    return response_buffer;
}

vector<uint8_t> DNSClient::queryDot(const vector<uint8_t>& query_packet, const string& server_ip, int timeout_sec)
{
    DOTClient dot_client(server_ip, 853);
    if (!_sni.empty())
        dot_client.setSni(_sni);

    try
    {
        if (!dot_client.connect(timeout_sec))
            return {};

        DNSMessage temp_message; // Criado apenas para passar para a interface do DOTClient
        temp_message.parseResponse(query_packet); // Re-parse para preencher o objeto

        if (!dot_client.sendQuery(temp_message, timeout_sec * 1000))
            return {};

        DNSMessage response_message;
        if (!dot_client.receiveResponse(response_message, timeout_sec * 1000))
            return {};
        
        // Reconstrói os bytes brutos da resposta para retornar
        return response_message.buildQuery(); 
    }
    catch (const exception& e)
    {
        cerr << "Erro durante a consulta DoT: " << e.what() << endl;
        return {};
    }
}

DNSClient::ResolutionState DNSClient::processResponse(const DNSMessage& response, vector<string>& current_nameservers, string& next_query_name)
{
    if (response.getRcode() != 0)
        return ResolutionState::Error;

    if (!response.answers.empty())
    {
        for (const auto& record : response.answers)
        {
            if (static_cast<DNSRecordType>(record.type) == DNSRecordType::CNAME)
            {
                // Reinicia a resolução a partir dos servidores raiz para o novo nome
                next_query_name = record.parsed_data;
                current_nameservers = {"198.41.0.4", "199.9.14.201", "192.33.4.12"};
                return ResolutionState::CnameRedirect;
            }
        }
        return ResolutionState::AnswerFound;
    }

    // Procura por delegações
    vector<string> delegated_ips = getDelegatedNsIps(response);
    if (!delegated_ips.empty())
    {
        current_nameservers = delegated_ips;
        return ResolutionState::Delegation;
    }
    
    return ResolutionState::Error;
}

vector<string> DNSClient::getDelegatedNsIps(const DNSMessage& response_message)
{
    vector<string> ips;
    for (const auto& record : response_message.additionals)
        if (static_cast<DNSRecordType>(record.type) == DNSRecordType::A ||
            static_cast<DNSRecordType>(record.type) == DNSRecordType::AAAA)
            ips.push_back(record.parsed_data);

    return ips;
}