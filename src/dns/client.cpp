#include "client.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <sys/time.h> 

using namespace std;

std::vector<uint8_t> DNSClient::send_recv_udp(
    const std::vector<uint8_t>& query, 
    const std::string& ns_ip, 
    uint16_t ns_port, 
    int timeout_sec) 
{
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw runtime_error("Falha ao criar o socket UDP.");
    }

    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        close(sockfd);
        throw runtime_error("Falha ao configurar timeout no socket.");
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(ns_port);

    if (inet_pton(AF_INET, ns_ip.c_str(), &servaddr.sin_addr) <= 0) {
        close(sockfd);
        throw invalid_argument("Endereço IP do servidor DNS inválido.");
    }

    if (sendto(sockfd, query.data(), query.size(), 0, 
               (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        close(sockfd);
        throw runtime_error("Falha ao enviar a consulta UDP (sendto).");
    }

    
    vector<uint8_t> buffer(512); 
    socklen_t len = sizeof(servaddr);
    ssize_t n = recvfrom(sockfd, buffer.data(), buffer.size(), 0, 
                         (struct sockaddr *)&servaddr, &len);

    
    if (n < 0) {
        close(sockfd);
        throw runtime_error("Timeout ou erro de rede: Não foi recebida resposta.");
    }

    buffer.resize(n);

    close(sockfd); 

    return buffer;
}


std::vector<uint8_t> DNSClient::resolve(const std::string& name, uint16_t qtype) {
    // Lógica recursiva
    // Teste:
    cout << "Iniciando resolução para " << name << " (QTYPE: " << qtype << ")" << endl;

    DNSMensagem msg;
    msg.configurarConsulta(name, qtype);
    vector<uint8_t> query = msg.montarQuery();

    const string root_ip = "198.41.0.4";
    const uint16_t root_port = 53;

    try {
        vector<uint8_t> resposta_bytes = send_recv_udp(query, root_ip, root_port, 5);
        cout << "Resposta do Root Server recebida. Tamanho: " << resposta_bytes.size() << " bytes." << endl;
        // falta a função msg.parseResposta(resposta_bytes) para decodificar
        return resposta_bytes;
    } catch (const exception& e) {
        cerr << "ERRO NA COMUNICAÇÃO: " << e.what() << endl;
        return {};
    }
}