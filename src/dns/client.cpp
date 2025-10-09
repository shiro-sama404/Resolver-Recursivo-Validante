#include "client.h"
#include "dns_mensagem.h" 
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
    
    if (sockfd < 0) 
    {
        throw runtime_error("Nao foi possivel criar o socket UDP.");
    }

    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) 
    {
        close(sockfd);
        throw runtime_error("Nao foi possivel configurar timeout no socket.");
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(ns_port);

    if (inet_pton(AF_INET, ns_ip.c_str(), &servaddr.sin_addr) <= 0) 
    {
        close(sockfd);
        throw invalid_argument("Endereço IP do servidor DNS invalido.");
    }

    if (sendto(sockfd, query.data(), query.size(), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    {
        close(sockfd);
        throw runtime_error("Nao foi possivel enviar a consulta UDP.");
    }

    
    vector<uint8_t> buffer(512); 
    socklen_t len = sizeof(servaddr);
    ssize_t n = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&servaddr, &len);

    
    if (n < 0) 
    {
        close(sockfd);
        throw runtime_error("Sem resposta. Timeout ou erro de rede.");
    }

    buffer.resize(n);

    close(sockfd); 

    return buffer;
}


std::vector<uint8_t> DNSClient::resolve(const std::string& name, uint16_t qtype) 
{
    vector<string> nameservers = {"198.41.0.4", "199.9.14.201", "192.33.4.12"}; 
    
    string nome_a_resolver = name;

    for (int i = 0; i < 30; ++i) 
    {
        cout << "--- Iteracao " << i+1 << ": Resolvendo " << nome_a_resolver << " usando NS: " << nameservers[0] << " ---" << endl;

        DNSMensagem consulta_msg;
        consulta_msg.configurarConsulta(nome_a_resolver, qtype);
        vector<uint8_t> query = consulta_msg.montarQuery();

        vector<uint8_t> resposta_bytes;
        try 
        {
            resposta_bytes = send_recv_udp(query, nameservers[0], 53, 5);
        } catch (const exception& e) 
        {
            cerr << "Erro ao comunicar com " << nameservers[0] << ": " << e.what() << endl;
            
            if (nameservers.size() > 1) 
            {
                nameservers.erase(nameservers.begin());
                continue;
            }
            return {}; 
        }

        DNSMensagem resposta_msg;
        resposta_msg.parseResposta(resposta_bytes); /
        resposta_msg.imprimirResposta(); 

        if (resposta_msg.cabecalho.ancount > 0) //Se contiver a resposta
        {
            for (const auto& rr : resposta_msg.respostas) 
            {
                if (rr.tipo == qtype) 
                { 
                    cout << "Resposta final encontrada." << endl;
                    return resposta_bytes; 
                }
                if (rr.tipo == 5) 
                {
                    nome_a_resolver = rr.resposta_parser; 
                    cout << "Redirecionando CNAME para: " << nome_a_resolver << endl;
                    nameservers = {"198.41.0.4"};
                    goto proxima_iteracao; 
                }
            }
        }
        if (resposta_msg.cabecalho.nscount > 0) //Se a resposta estiver na seção de autoridade
        {
            vector<string> proximos_nameservers_ips;

            for (const auto& rr_ns : resposta_msg.autoridades) 
            {
                if (rr_ns.tipo == 2) 
                { 
                    string ns_hostname = rr_ns.resposta_parser;
                    
                    cout << "Delegacao para: " << ns_hostname << ". Resolvendo o IP..." << endl;

                    DNSClient resolver_auxiliar; 
                    vector<uint8_t> ip_ns_bytes = resolver_auxiliar.resolve(ns_hostname, 1);
            
                     if (!ip_ns_bytes.empty()) 
                    {
                        DNSMensagem ip_ns_msg;
                       
                        ip_ns_msg.parseResposta(ip_ns_bytes);
                       
                        if (ip_ns_msg.cabecalho.ancount > 0) 
                        {                
                           proximos_nameservers_ips.push_back(ip_ns_msg.respostas[0].resposta_parser);
                           cout << "IP de " << ns_hostname << " encontrado: " << ip_ns_msg.respostas[0].resposta_parser << endl;
                        }
                    }
                }
            }       
            if (!proximos_nameservers_ips.empty())
            {
                nameservers = proximos_nameservers_ips;
                continue; 
            }

        }
        if (resposta_msg.cabecalho.ancount == 0 && (resposta_msg.cabecalho.flags & 0x000F) == 0) //Em caso de NODATA
            if (resposta_msg.cabecalho.nscount > 0) 
                for (const auto& rr : resposta_msg.autoridades) 
                    if (rr.tipo == 6) 
                    { 
                        cout << "Nao ha dados para o tipo de registro solicitado nesse dominio." << endl;
                        return resposta_bytes; 
                    }

        if ((resposta_msg.cabecalho.flags & 0x000F) == 3) // Em caso de NXDOMAIN 
        {
            cout << "Dominio nao encontrado." << endl;
            return resposta_bytes; 
        }
        
        cout << "Resposta nao encontrada (ou delegacao nao seguida)." << endl;
        return {};

        proxima_iteracao:; 
    }

    cout << "O Limite de iteracoes foi atingido." << endl;
    return {};
}