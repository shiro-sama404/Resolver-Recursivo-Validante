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
/*
string lerNome(const vector<uint8_t>& dados, size_t& pos) {
    string nome;          
    size_t pos_original = pos;     
    bool ponteiro = false;  

    while (pos < dados.size()) {
        uint8_t tam = dados[pos++];  

        bool isPointer = (ponteiro & 0xC0) == 0xC0; // se os 2 bits mais altos forem 11, é ponteiro, se não é o tamanho da palavra
        if (isPointer) {
            uint8_t parteAlta = ponteiro & 0x3F;   // 6 bits baixos do primeiro byte do ponteiro
            uint8_t parteBaixa = dados[pos++];              // segundo byte do ponteiro
            size_t posPonteiro = (parteAlta << 8) | parteBaixa; // posição real no pacote

            if (!seguiuPonteiro) posOriginal = pos;  // salva posição caso seja ponteiro pela primeira vez
            pos = posPonteiro;                        // vai para a posição indicada pelo ponteiro
            seguiuPonteiro = true;
            continue;
        }

        // Se tamanho == 0, chegamos ao final do nome
        if (tamanhoOuPonteiro == 0) break;

        // adiciona ponto entre labels se já houver algo no nome
        if (!nomeCompleto.empty()) nomeCompleto += '.';

        // lê os caracteres do label
        for (int i = 0; i < tamanhoOuPonteiro && pos < dados.size(); ++i)
            nomeCompleto += static_cast<char>(dados[pos++]);
    }

    // Se seguimos algum ponteiro, voltamos à posição original para continuar o parse
    if (seguiuPonteiro) pos = posOriginal;

    return nomeCompleto;
}*/

struct RegistroRecursos {
    string nome;
    uint16_t tipo;
    uint16_t classe;
    uint32_t ttl;
    uint16_t rdlen;
    vector<uint8_t> rdata; 
};



void DNSMensagem::parseResposta(const vector<uint8_t>& dados) {
    if (dados.size() < 12) {
        cout << "Pacote DNS inválido (muito pequeno)." << endl;
        return;
    }

    size_t pos = 0;

    // Cabeçalho
    cabecalho.id      = lerUint16(dados, pos);
    cabecalho.flags   = lerUint16(dados, pos);
    cabecalho.qdcount = lerUint16(dados, pos);
    cabecalho.ancount = lerUint16(dados, pos);
    cabecalho.nscount = lerUint16(dados, pos);
    cabecalho.arcount = lerUint16(dados, pos);

    // Pergunta
    pergunta.qname  = lerNome(dados, pos);
    pergunta.qtype  = lerUint16(dados, pos);
    pergunta.qclass = lerUint16(dados, pos);

    // Respostas
    respostas.clear(); // limpa qualquer resposta anterior
    for (int i = 0; i < cabecalho.ancount; ++i) {
        RegistroRecursos reg;
        reg.nome   = lerNome(dados, pos);
        reg.tipo   = lerUint16(dados, pos);
        reg.classe = lerUint16(dados, pos);
        reg.ttl    = lerUint32(dados, pos);
        reg.rdlen  = lerUint16(dados, pos);

        // copia os dados da resposta
        reg.rdata.assign(dados.begin() + pos, dados.begin() + pos + reg.rdlen);
        pos += reg.rdlen;

        respostas.push_back(reg);

        // impressão opcional
        cout << "Nome: " << reg.nome << "  Tipo: " << reg.tipo
             << "  Classe: " << reg.classe << "  TTL: " << reg.ttl << endl;

        if (reg.tipo == 1 && reg.rdlen == 4) { // tipo A → IPv4
            cout << "Endereço: "
                 << (int)reg.rdata[0] << "."
                 << (int)reg.rdata[1] << "."
                 << (int)reg.rdata[2] << "."
                 << (int)reg.rdata[3] << endl;
        }
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
