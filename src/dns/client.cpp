#include "client.h"
#include "dns_mensagem.h" 
#include <iostream>
#include <cstring>
#include <sys/time.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

using namespace std;

std::vector<uint8_t> DNSClient::send_recv_udp(const std::vector<uint8_t>& pctC, const std::string& nomeSer, uint16_t pServ, int tempoTO) 
{

    int descritor = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (descritor < 0) 
        throw runtime_error("Desculpe, mas nao foi possivel criar o socket UDP.");
    

    struct timeval tEspera;
    tEspera.tv_sec = tempoTO;
    tEspera.tv_usec = 0;
    
    int resultadoOpt = setsockopt(descritor, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tEspera, sizeof(tEspera));
    if (resultadoOpt < 0) {
        close(descritor);
        throw runtime_error("\nNao foi possivel configurar timeout no socket.");
    }

    struct sockaddr_in endServ;
    memset(&endServ, 0, sizeof(endServ));
    endServ.sin_family = AF_INET;
    endServ.sin_port = htons(pServ);

    int resultadoPton = inet_pton(AF_INET, nomeSer.c_str(), &endServ.sin_addr);

    if (resultadoPton <= 0) 
    {
        close(descritor);
        throw invalid_argument("\nEndereço IP do servidor DNS invalido.");
    }

    int resultadoSend = sendto(descritor, pctC.data(), pctC.size(), 0, (const struct sockaddr *)&endServ, sizeof(endServ));

    if (resultadoSend < 0) {
        close(descritor);
        throw runtime_error("\nNao foi possivel enviar a consulta UDP.");
    }

    vector<uint8_t> bufferR(512); 
    socklen_t tamEnd = sizeof(endServ);
    
    ssize_t bytesR = recvfrom(descritor, bufferR.data(), bufferR.size(), 0, (struct sockaddr *)&endServ, &tamEnd);

    if (bytesR < 0) {
        close(descritor);
        throw runtime_error("\nSem resposta. Houve Timeout ou um erro de rede.");
    }

    bufferR.resize(bytesR);
    close(descritor); 
    return bufferR;
}


std::vector<uint8_t> DNSClient::resolve(const std::string& name, uint16_t qtype) 
{
    vector<string> nomeServ = {"198.41.0.4", "199.9.14.201", "192.33.4.12"}; 
    
    string nomeR = name;

    for (int i = 0; i < 30; ++i) 
    {
        cout << "--- Iteracao " << i+1 << ": Resolvendo " << nomeR << " usando NS: " << nomeServ[0] << " ---" << endl;

        DNSMensagem msgConsulta;
        msgConsulta.configurarConsulta(nomeR, qtype);
        vector<uint8_t> pctC = msgConsulta.montarQuery();

        vector<uint8_t> bytesResp;
        
        try 
        {
            bytesResp = send_recv_udp(pctC, nomeServ[0], 53, 5);
        } catch (const exception& e) 
        {
            cerr << "Erro ao comunicar com " << nomeServ[0] << ": " << e.what() << endl;
            
            if (nomeServ.size() > 1) 
            {
                nomeServ.erase(nomeServ.begin());
                continue;
            }
            return {}; 
        }

        DNSMensagem msgResposta;
        msgResposta.parseResposta(bytesResp); 
        msgResposta.imprimirResposta(); 

        if (msgResposta.cabecalho.ancount > 0) //Se contiver a resposta
        {
            for (const auto& rr : msgResposta.respostas) 
            {
                if (rr.tipo == qtype) 
                { 
                    cout << "Resposta final encontrada." << endl;
                    return bytesResp; 
                }
                if (rr.tipo == 5) 
                {
                    nomeR = rr.resposta_parser; 
                    cout << "Redirecionando CNAME para: " << nomeR << endl;
                    nomeServ = {"198.41.0.4"};
                    goto prox; 
                }
            }
        }
        if (msgResposta.cabecalho.nscount > 0) //Se a resposta estiver na seção de autoridade
        {
            vector<string> ipsProxServ;

            for (const auto& rr_ns : msgResposta.autoridades) 
            {
                if (rr_ns.tipo == 2) 
                { 
                    string nHs = rr_ns.resposta_parser;
                    
                    cout << "Delegacao para: " << nHs << ". Resolvendo o IP..." << endl;

                    DNSClient aux; 
                    vector<uint8_t> bEndNS = aux.resolve(nHs, 1);
            
                     if (!bEndNS.empty()) 
                    {
                        DNSMensagem msgIpNS;
                       
                        msgIpNS.parseResposta(bEndNS);
                       
                        if( msgIpNS.cabecalho.ancount > 0) 
                        {                
                           ipsProxServ.push_back(msgIpNS.respostas[0].resposta_parser);
                           cout << "IP referente a: " << nHs << " encontrado:msgIpNS.respostas[0].resposta_parser" << endl;
                        }
                    }
                }
            }       
            if (!ipsProxServ.empty())
            {
                nomeServ = ipsProxServ;
                continue; 
            }

        }
        if (msgResposta.cabecalho.ancount == 0 && (msgResposta.cabecalho.flags & 0x000F) == 0) //Em caso de NODATA
            if (msgResposta.cabecalho.nscount > 0) 
                for (const auto& rr : msgResposta.autoridades) 
                    if (rr.tipo == 6) 
                    { 
                        cout << "Nao ha dados para o tipo de registro solicitado nesse dominio." << endl;
                        return bytesResp; 
                    }

        if ((msgResposta.cabecalho.flags & 0x000F) == 3) // Em caso de NXDOMAIN 
        {
            cout << "Dominio nao encontrado." << endl;
            return bytesResp; 
        }
        
        cout << "Resposta nao encontrada (ou delegacao nao seguida)." << endl;
        return {};

        prox:; 
    }

    cout << "O Limite de iteracoes foi atingido." << endl;
    return {};
}