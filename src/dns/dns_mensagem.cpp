/* Còdigo Fernanda*/
#include "dns_mensagem.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip> 
#include <cstdlib> 

using namespace std;

DNSMensagem::DNSMensagem() {
    cabecalho.id = 0;
    cabecalho.flags = 0x0100;  
    cabecalho.qdcount = 1;
    cabecalho.ancount = 0;
    cabecalho.nscount = 0;
    cabecalho.arcount = 0;

    pergunta.qname = "";
    pergunta.qtype = 1;  
    pergunta.qclass = 1;  
}

void DNSMensagem::configurarConsulta(const string& nome, uint16_t tipo) {
    pergunta.qname = nome;
    pergunta.qtype = tipo;
    pergunta.qclass = 1;      
    cabecalho.id = rand() % 65536; 
}


// Função auxiliar 
void DNSMensagem::addUint16(vector<uint8_t>& pacote, uint16_t valor) {
    pacote.push_back(valor >> 8);     
    pacote.push_back(valor & 0xFF);   
}

void DNSMensagem::addPergunta(vector<uint8_t>& pacote) {
    size_t inicio = 0;
    size_t ponto = 0;
    string nome = pergunta.qname;

    while ((ponto = nome.find('.', inicio)) != string::npos) {
        uint8_t tamanho = ponto - inicio;
        pacote.push_back(tamanho);
        
        for (size_t i = inicio; i < ponto; ++i)
            pacote.push_back(nome[i]);
        inicio = ponto + 1;
    }

    uint8_t tamanho = nome.size() - inicio;
    pacote.push_back(tamanho);
    for (size_t i = inicio; i < nome.size(); ++i)
        pacote.push_back(nome[i]);

    pacote.push_back(0);
    addUint16(pacote, pergunta.qtype);
    addUint16(pacote, pergunta.qclass);
}

vector<uint8_t> DNSMensagem::montarQuery() {
    vector<uint8_t> pacote;

    addUint16(pacote, cabecalho.id);
    addUint16(pacote, cabecalho.flags);
    addUint16(pacote, cabecalho.qdcount);
    addUint16(pacote, cabecalho.ancount); // tamanho da resposta do servidor
    addUint16(pacote, cabecalho.nscount);
    addUint16(pacote, cabecalho.arcount);

    addPergunta(pacote);

    return pacote;
}


uint16_t lerUint16(const std::vector<uint8_t>& dados, size_t& pos) {
    if (pos + 2 > dados.size()) 
        throw std::runtime_error("Erro ao ler uint16");

    uint16_t valor = 0;

    for (int i = 0; i < 2; ++i) {
        //  | -> operador bit a bit, usado para combinar bytes em um único número.
        valor = (valor << 8) | dados[pos + i];
    }

    pos += 2;
    return valor;
}

// criei uma função diferente para ler dados de 32 bits porque ele só é lido uma vez
// se fosse criar uma função geral, teria que ficar convertendo os dados de 16 bits toda vez que chamasse
// afinal, 16 bits cabem em 32, mas 32 não cabem em 16
uint32_t lerUint32(const std::vector<uint8_t>& dados, size_t& pos) {
    if (pos + 4 > dados.size()) 
        throw std::runtime_error("Erro ao ler uint32");
    uint32_t valor = 0;

    for (int i = 0; i < 4; ++i) {
        valor = (valor << 8) | dados[pos + i];
    }

    pos += 4;
    return valor;
}

string lerNome(const vector<uint8_t>& dados, size_t& pos) {
    string nome;
    size_t pos_original = pos;
    bool pular_pos = false;

    while (pos < dados.size()) {
        if (pos >= dados.size())
            throw std::runtime_error("Erro ao ler nome: pacote DNS truncado");
        uint8_t len = dados[pos];

        if ((len & 0xC0) == 0xC0) {
            if (pos >= dados.size())
                throw std::runtime_error("Erro ao ler ponteiro de nome DNS");

            if (pos + 1 >= dados.size()) 
                break;
            
            uint16_t offset = ((len & 0x3F) << 8) | dados[pos + 1];
            pos += 2;
            
            if (!pular_pos) {
                pos_original = pos;
                pular_pos = true;
            }
            pos = offset;
            continue;
        }

        pos++;
        if (len == 0) break;

        if (!nome.empty())
            nome += '.';

        for (int i = 0; i < len; ++i)
            nome += static_cast<char>(dados[pos++]);
    }

    if (!pular_pos) 
        return nome;
    
    pos = pos_original;
    
    return nome;
}


void DNSMensagem::lerCabecalho(const std::vector<uint8_t>& dados, size_t& pos) {
    cabecalho.id      = lerUint16(dados, pos);
    cabecalho.flags   = lerUint16(dados, pos);
    cabecalho.qdcount = lerUint16(dados, pos);
    cabecalho.ancount = lerUint16(dados, pos);
    cabecalho.nscount = lerUint16(dados, pos);
    cabecalho.arcount = lerUint16(dados, pos);
}

void DNSMensagem::lerPergunta(const std::vector<uint8_t>& dados, size_t& pos) {
    pergunta.qname  = lerNome(dados, pos);
    pergunta.qtype  = lerUint16(dados, pos);
    pergunta.qclass = lerUint16(dados, pos);
}

// funçoes de decodificaçao

void DNSMensagem::decodeA(ResourceRecords& rr) {
    if (rr.rdlen == 4) {
        rr.resposta_parser = std::to_string(rr.rdata[0]) + "." +
                             std::to_string(rr.rdata[1]) + "." +
                             std::to_string(rr.rdata[2]) + "." +
                             std::to_string(rr.rdata[3]);
    } else {
        rr.resposta_parser = "Registro A inválido";
    }
}

void DNSMensagem::decodeAAAA(ResourceRecords& rr) {
    if (rr.rdlen == 16) {
        char buf[40];
        sprintf(buf, "%x:%x:%x:%x:%x:%x:%x:%x",
                (rr.rdata[0] << 8) | rr.rdata[1],
                (rr.rdata[2] << 8) | rr.rdata[3],
                (rr.rdata[4] << 8) | rr.rdata[5],
                (rr.rdata[6] << 8) | rr.rdata[7],
                (rr.rdata[8] << 8) | rr.rdata[9],
                (rr.rdata[10] << 8) | rr.rdata[11],
                (rr.rdata[12] << 8) | rr.rdata[13],
                (rr.rdata[14] << 8) | rr.rdata[15]);
        rr.resposta_parser = std::string(buf);
    } else {
        rr.resposta_parser = "Registro AAAA inválido";
    }
}

void DNSMensagem::decodeCNAME(ResourceRecords& rr) {
    size_t pos_local = 0;
    rr.resposta_parser = lerNome(rr.rdata, pos_local);
}

// descobre quem é o servidor autoritativo do dominio
// ou seja, dada uma URL
// ele descobre pra quem tem que perguntar para descobrir o IP
void DNSMensagem::decodeNS(ResourceRecords& rr) {
    size_t pos_local = 0;
    rr.resposta_parser = lerNome(rr.rdata, pos_local);
}

void DNSMensagem::lerRespostas(const std::vector<uint8_t>& dados, size_t& pos, int count) {
    for (int i = 0; i < count; ++i) {
        ResourceRecords rr;
        rr.nome   = lerNome(dados, pos);
        rr.tipo   = lerUint16(dados, pos);
        rr.classe = lerUint16(dados, pos);
        rr.ttl    = lerUint32(dados, pos);
        rr.rdlen  = lerUint16(dados, pos);

        rr.rdata.assign(dados.begin() + pos, dados.begin() + pos + rr.rdlen);

        switch (rr.tipo) {
            case 1:  
                decodeA(rr);
                break;
            case 2:  
                decodeNS(rr);
                break;
            case 5:  
                decodeCNAME(rr);
                break;
            case 6:  
                decodeSOA(rr);
                break;
            case 15: 
                decodeMX(rr);
                break;
            case 16:  
                decodeTXT(rr); 
                break;
            case 28: 
                decodeAAAA(rr);
                break;
            case 33:  
                decodeSRV(rr); 
                break;
            case 41: 
                decodeOPT(rr);
                break;
            case 43: 
                decodeDS(rr);
                break;
            case 46: 
                decodeRRSIG(rr);
                break;
            case 48: 
                decodeDNSKEY(rr);
                break;
            default:
                rr.resposta_parser = "Tipo de registro não tratado: " + std::to_string(rr.tipo);
                break;
        }

        pos += rr.rdlen;
        respostas.push_back(rr);
        cout << "---------------------------\n";
    }
}


void DNSMensagem::parseResposta(const vector<uint8_t>& dados) {
   if (dados.size() < 12) {
        throw std::runtime_error("Pacote DNS inválido (muito pequeno)");
    }

    size_t pos = 0;

    // Cabeçalho
    cabecalho.id      = lerUint16(dados, pos);
    cabecalho.flags   = lerUint16(dados, pos);
    cabecalho.qdcount = lerUint16(dados, pos);
    cabecalho.ancount = lerUint16(dados, pos);
    cabecalho.nscount = lerUint16(dados, pos);
    cabecalho.arcount = lerUint16(dados, pos);

    // Perguntas
    pergunta.qname  = lerNome(dados, pos);
    pergunta.qtype  = lerUint16(dados, pos);
    pergunta.qclass = lerUint16(dados, pos);

    // Respostas
    for (int i = 0; i < cabecalho.ancount; ++i) {
        string nome = lerNome(dados, pos);
        uint16_t tipo   = lerUint16(dados, pos);
        uint16_t classe = lerUint16(dados, pos);
        uint32_t ttl    = lerUint32(dados, pos);
        uint16_t rdlen  = lerUint16(dados, pos);

        cout << "Nome: " << nome << "  Tipo: " << tipo
             << "  Classe: " << classe << "  TTL: " << ttl << endl;

        // Tipo A → IPv4
        if (tipo == 1 && rdlen == 4) {
            cout << "Endereço: "
                 << (int)dados[pos] << "."
                 << (int)dados[pos + 1] << "."
                 << (int)dados[pos + 2] << "."
                 << (int)dados[pos + 3] << endl;
        }

        pos += rdlen;
        cout << "---------------------------\n";
    }
}


void DNSMensagem::imprimirResposta() {
    cout << "\n========= CABEÇALHO DNS =========\n";
    cout << "ID:        " << cabecalho.id << endl;
    cout << "Flags:     0x" << hex << setw(4) << setfill('0') << cabecalho.flags << dec << endl;
    cout << "QDCOUNT:   " << cabecalho.qdcount << endl;
    cout << "ANCOUNT:   " << cabecalho.ancount << endl;
    cout << "NSCOUNT:   " << cabecalho.nscount << endl;
    cout << "ARCOUNT:   " << cabecalho.arcount << endl;

    cout << "\n========= PERGUNTA =========\n";
    cout << "Domínio:   " << pergunta.qname << endl;
    cout << "Tipo:      " << pergunta.qtype << endl;
    cout << "Classe:    " << pergunta.qclass << endl;
}
