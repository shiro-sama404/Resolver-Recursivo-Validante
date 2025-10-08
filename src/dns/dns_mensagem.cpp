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
    addUint16(pacote, cabecalho.ancount);
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
