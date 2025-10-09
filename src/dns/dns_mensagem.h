/* Còdigo Fernanda*/
#ifndef DNSMENSAGEM_H
#define DNSMENSAGEM_H

#include <string>
#include <vector>
#include <cstdint>

struct CabecalhoDNS {
    uint16_t id;      
    uint16_t flags;    
    uint16_t qdcount;  
    uint16_t ancount;  
    uint16_t nscount;  
    uint16_t arcount;  
};

// o site em si que ele quer pesquisar
struct PerguntaDNS { 
    std::string qname;
    uint16_t qtype;    
    uint16_t qclass;   
};

// ResourceRecurses
struct RegistroRecursos {
    std::string nome;
    uint16_t tipo;
    uint16_t classe;
    uint32_t ttl;
    uint16_t rdlen;
    std::vector<uint8_t> rdata; 
    std::string resposta_parser; // para guardar a resposta decodificada do parser para acessar mais tarde
};

class DNSMensagem {
public:
    CabecalhoDNS cabecalho;
    PerguntaDNS pergunta;
    
    //uint16_t edns_udp_size = 512;      

    DNSMensagem();
    
    void configurarConsulta(const std::string& nome, uint16_t tipo);
    
    std::vector<uint8_t> montarQuery();
    
    std::vector<RegistroRecursos> respostas;
    std::vector<RegistroRecursos> autoridades;
    std::vector<RegistroRecursos> adicionais;

    void parseResposta(const std::vector<uint8_t>& dados);
    
    void imprimirResposta();

private:
    void addUint16(std::vector<uint8_t>& pacote, uint16_t valor);
    void addPergunta(std::vector<uint8_t>& pacote); 
};

#endif





