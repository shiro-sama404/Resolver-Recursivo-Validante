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


uint16_t lerUint16(const std::vector<uint8_t>& dados, size_t& pos) {
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
    uint32_t valor = 0;

    for (int i = 0; i < 4; ++i) {
        valor = (valor << 8) | dados[pos + i];
    }

    pos += 4;
    return valor;
}

string lerNome(const vector<uint8_t>& dados, size_t& pos) {
    string nome;

    while (pos < dados.size()) {
        uint8_t len = dados[pos++];

        if (len == 0) break;

        if (!nome.empty()) 
            nome += '.';

        for (int i = 0; i < len; ++i)
            nome += static_cast<char>(dados[pos++]);
    }
    return nome;
}

void DNSMensagem::parseResposta(const vector<uint8_t>& dados) {
    if (dados.size() < 12) {
        cout << "Pacote DNS inválido (muito pequeno)." << endl;
        return;
    }

    size_t pos = 0;

    cabecalho.id      = lerUint16(dados, pos);
    cabecalho.flags   = lerUint16(dados, pos);
    cabecalho.qdcount = lerUint16(dados, pos);
    cabecalho.ancount = lerUint16(dados, pos);
    cabecalho.nscount = lerUint16(dados, pos);
    cabecalho.arcount = lerUint16(dados, pos);

    pergunta.qname  = lerNome(dados, pos);
    pergunta.qtype  = lerUint16(dados, pos);
    pergunta.qclass = lerUint16(dados, pos);

    for (int i = 0; i < cabecalho.ancount; ++i) {
        string nome = lerNome(dados, pos);
        uint16_t tipo   = lerUint16(dados, pos);
        uint16_t classe = lerUint16(dados, pos);
        uint32_t ttl    = lerUint32(dados, pos); // tempo que o dado pode ficar armazenado em cache
        uint16_t rdlen  = lerUint16(dados, pos); // tamanho da resposta

        cout << "Nome: " << nome << "  Tipo: " << tipo
             << "  Classe: " << classe << "  TTL: " << ttl << endl;

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
