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
};

class DNSMensagem {
public:
    CabecalhoDNS cabecalho;
    PerguntaDNS pergunta;
    std::vector<RegistroRecursos> respostas;

    DNSMensagem();
    
    void configurarConsulta(const std::string& nome, uint16_t tipo);
    
    std::vector<uint8_t> montarQuery();
    
    void parseResposta(const std::vector<uint8_t>& dados);
    
    void imprimirResposta();

private:
    void addUint16(std::vector<uint8_t>& pacote, uint16_t valor);
    void addPergunta(std::vector<uint8_t>& pacote); 
};

#endif


