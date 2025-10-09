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
#define MAX_DEPTH 30

using namespace std;

std::vector<uint8_t> DNSClient::udp(const std::vector<uint8_t>& pctC, const std::string& nomeSer, uint16_t pServ, int tempoTO) 
{

    int descritor = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (descritor < 0) 
        throw runtime_error("Erro em: udp()");
    

    struct timeval tEspera;
    tEspera.tv_sec = tempoTO;
    tEspera.tv_usec = 0;
    
    int resultOpt = setsockopt(descritor, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tEspera, sizeof(tEspera));

    struct sockaddr_in endServ;
    memset(&endServ, 0, sizeof(endServ));
    endServ.sin_family = AF_INET;
    endServ.sin_port = htons(pServ);

    int resultPton = inet_pton(AF_INET, nomeSer.c_str(), &endServ.sin_addr);

    if (resultPton <= 0) 
        throw invalid_argument("\nEndereço IP invalido.");
    

    int resultEnv = sendto(descritor, pctC.data(), pctC.size(), 0, (const struct sockaddr *)&endServ, sizeof(endServ));

    vector<uint8_t> bufferR(512); 
    socklen_t tamEnd = sizeof(endServ);
    
    ssize_t bytesR = recvfrom(descritor, bufferR.data(), bufferR.size(), 0, (struct sockaddr *)&endServ, &tamEnd);

    if ((resultOpt >= 0) && (resultEnv >= 0) && (bytesR >= 0))
        ;
    else
        throw runtime_error("Erro em udp()");

    bufferR.resize(bytesR);
    close(descritor); 
    return bufferR;
}


std::vector<uint8_t> DNSClient::resolvedor(const std::string& name, uint16_t qtype) 
{
    vector<string> nomeServ = {"198.41.0.4", "199.9.14.201", "192.33.4.12"}; 
    
    string nomeR = name;
    
    int qtd = 0;

    while(qtd < MAX_DEPTH)
    {
        qtd++;

        DNSMensagem msgConsulta;
        msgConsulta.configurarConsulta(nomeR, qtype);
        vector<uint8_t> pctC = msgConsulta.montarQuery();

        vector<uint8_t> bytesResp;
        
        try 
        {
            bytesResp = udp(pctC, nomeServ[0], 53, 5);
            
            uint16_t flags = bytesResp[2] * 256 + bytesResp[3];
            bool respostaTruncada = (flags >> 9) & 1; 

            if (respostaTruncada) 
            {
                cout << "\nResposta UDP truncada.\n" << endl;

                int descritorTCP = socket(AF_INET, SOCK_STREAM, 0);
                
                if (descritorTCP < 0) 
                    throw runtime_error("Falgou em criar o socket TCP.\n");

                
                struct sockaddr_in endServTCP;
                memset(&endServTCP, 0, sizeof(endServTCP));
                endServTCP.sin_family = AF_INET;
                endServTCP.sin_port = htons(53); 
                inet_pton(AF_INET, nomeServ[0].c_str(), &endServTCP.sin_addr);

                if (connect(descritorTCP, (struct sockaddr*)&endServTCP, sizeof(endServTCP)) < 0) 
                    throw runtime_error("Erro ao conectar via TCP.\n");
            

                uint16_t tamanho_net = htons(pctC.size());

                vector<uint8_t> pctTCP;

                pctTCP.push_back(tamanho_net >> 8);
                pctTCP.push_back(tamanho_net & 0xFF);
                pctTCP.insert(pctTCP.end(), pctC.begin(), pctC.end());
                
                send(descritorTCP, pctTCP.data(), pctTCP.size(), 0);

                vector<uint8_t> buffer_tamanho(2);

                recv(descritorTCP, buffer_tamanho.data(), 2, 0);

                uint16_t tamRsp = buffer_tamanho[0] * 256 + buffer_tamanho[1];
                
                bytesResp.resize(tamRsp); 

                recv(descritorTCP, bytesResp.data(), tamRsp, 0);

                close(descritorTCP);
            }

        } catch (const exception& e) 
        {
            cerr << "Erro em: " << nomeServ[0] << ": " << e.what() << endl;
            
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

        if (msgResposta.cabecalho.ancount > 0)
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
        if (msgResposta.cabecalho.nscount > 0) 
        {
            vector<string> ipsProxServ;

            for (const auto& rr_ns : msgResposta.autoridades) 
            {
                if (rr_ns.tipo == 2) 
                { 
                    string nHs = rr_ns.resposta_parser;

                    DNSClient aux; 
                    vector<uint8_t> bEndNS = aux.resolvedor(nHs, 1);
            
                     if (!bEndNS.empty()) 
                    {
                        DNSMensagem msgIpNS;
                       
                        msgIpNS.parseResposta(bEndNS);
                       
                        if( msgIpNS.cabecalho.ancount > 0)          
                           ipsProxServ.push_back(msgIpNS.respostas[0].resposta_parser);
                         
                    }
                }
            }       
            if (!ipsProxServ.empty())
            {
                nomeServ = ipsProxServ;
                continue; 
            }

        }
        if (msgResposta.cabecalho.ancount == 0 && (msgResposta.cabecalho.flags & 0x000F) == 0) 
            if (msgResposta.cabecalho.nscount > 0) 
                for (const auto& rr : msgResposta.autoridades) 
                    if (rr.tipo == 6) 
                    { 
                        cout << "Sem dados para o registro solicitado no dominio." << endl;
                        return bytesResp; 
                    }

        if ((msgResposta.cabecalho.flags & 0x000F) == 3) 
        {
            cout << "Dominio nao encontrado." << endl;
            return bytesResp; 
        }
        
        cout << "Resposta nao encontrada (ou delegacao nao seguida)." << endl;
        return {};

        prox:; 
    }

    cout << "Limite de iteracoes atingido." << endl;
    return {};
}