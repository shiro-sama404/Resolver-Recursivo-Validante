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

// ResourceRecords
struct ResourceRecords {
    std::string nome;
    uint16_t tipo;
    uint16_t classe;
    uint32_t ttl;
    uint16_t rdlen;
    std::vector<uint8_t> rdata; 
    std::string resposta_parser; // para guardar a resposta decodificada do parser para acessar mais tarde
};

struct EDNSOption {
        uint16_t code;
        std::vector<uint8_t> data;
};

class DNSMensagem {

public:
    CabecalhoDNS cabecalho;
    PerguntaDNS pergunta;      

    DNSMensagem();
    
    void configurarConsulta(const std::string& nome, uint16_t tipo);
    
    std::vector<uint8_t> montarQuery();
    
    std::vector<ResourceRecords> respostas;
    std::vector<ResourceRecords> autoridades;
    std::vector<ResourceRecords> adicionais;

    std::vector<EDNSOption> edns_options; // opções extras

    void parseResposta(const std::vector<uint8_t>& dados);
    
    void imprimirResposta();

private:
    void addUint16(std::vector<uint8_t>& pacote, uint16_t valor);
    void addPergunta(std::vector<uint8_t>& pacote); 
    void lerCabecalho(const std::vector<uint8_t>& dados, size_t& pos);
    void lerPergunta(const std::vector<uint8_t>& dados, size_t& pos);
    void lerRespostas(const std::vector<uint8_t>& dados, size_t& pos, int count);
    void lerAutoridade(const std::vector<uint8_t>& dados, size_t& pos, int count);
    void lerAdicional(const std::vector<uint8_t>& dados, size_t& pos, int count);
    void decodeA(ResourceRecords& rr);
    void decodeAAAA(ResourceRecords& rr);
    void decodeCNAME(ResourceRecords& rr);
    void decodeDS(ResourceRecords& rr);
    void decodeNS(ResourceRecords& rr);
    void decodeDNSKEY(ResourceRecords& rr);
    void decodeMX(ResourceRecords& rr);
    void decodeRRSIG(ResourceRecords& rr);
    void decodeSOA(ResourceRecords& rr);
    void decodeTXT(ResourceRecords& rr);
    void decodeOPT(ResourceRecords& rr);

    ResourceRecords lerRegistro(const std::vector<uint8_t>& dados, size_t& pos);
    uint16_t edns_udp_size = 512; // tamanho negociado
    uint8_t  edns_version = 0; // versão
    uint16_t edns_z = 0; // campos reservados

};

#endif





