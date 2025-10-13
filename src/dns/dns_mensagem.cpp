#include "dns_mensagem.hpp"

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


uint16_t lerUint16(const vector<uint8_t>& dados, size_t& pos) {
    
    if (pos + 2 > dados.size()) 
        throw runtime_error("Erro ao ler uint16");

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
uint32_t lerUint32(const vector<uint8_t>& dados, size_t& pos) {

    if (pos + 4 > dados.size()) 
        throw runtime_error("Erro ao ler uint32");
    
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
        uint8_t len = dados[pos];


        if ((len & 0xC0) == 0xC0) {
            if (pos + 1 >= dados.size())
                throw runtime_error("Erro ao ler ponteiro de nome DNS");

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
        if (len == 0)
            break; 

        if (!nome.empty())
            nome += '.';

        if (pos + len > dados.size())
            throw runtime_error("Erro ao ler nome: pacote DNS truncado");

        for (int i = 0; i < len; ++i) {
            nome += static_cast<char>(dados[pos++]);
        }
    }

    if (pular_pos) 
        pos = pos_original;  

    return nome;
}


      

void DNSMensagem::lerCabecalho(const vector<uint8_t>& dados, size_t& pos) {
    cabecalho.id      = lerUint16(dados, pos);
    cabecalho.flags   = lerUint16(dados, pos);
    cabecalho.qdcount = lerUint16(dados, pos);
    cabecalho.ancount = lerUint16(dados, pos);
    cabecalho.nscount = lerUint16(dados, pos);
    cabecalho.arcount = lerUint16(dados, pos);
}


void DNSMensagem::lerPergunta(const vector<uint8_t>& dados, size_t& pos) {
    pergunta.qname  = lerNome(dados, pos);
    pergunta.qtype  = lerUint16(dados, pos);
    pergunta.qclass = lerUint16(dados, pos);
}



// funçoes de decodificaçao

void DNSMensagem::decodeA(ResourceRecords& rr) {

    if (rr.rdlen == 4) {
        rr.resposta_parser = to_string(rr.rdata[0]) + "." +
                             to_string(rr.rdata[1]) + "." +
                             to_string(rr.rdata[2]) + "." +
                             to_string(rr.rdata[3]);
    } 
    else {
        rr.resposta_parser = "ERRO: Registro A inválido. Esperado 4 bytes, recebeu " + to_string(rr.rdlen);
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
        rr.resposta_parser = string(buf);
    } 
    else {
        rr.resposta_parser = "ERRO: Registro AAAA inválido. Esperado 16 bytes, recebeu " + to_string(rr.rdlen);
    }

}



void DNSMensagem::decodeCNAME(ResourceRecords& rr) {
    
    try { 
        size_t pos_local = 0;
        rr.resposta_parser = lerNome(rr.rdata, pos_local); 
    } 
    
    catch (const exception& erro) {
        rr.resposta_parser = "ERRO ao decodificar CNAME: " + string(erro.what());
    }
}



// descobre quem é o servidor autoritativo do dominio
// ou seja, dada uma URL
// ele descobre pra quem tem que perguntar para descobrir o IP
void DNSMensagem::decodeNS(ResourceRecords& rr) {
   
    try {
        size_t pos_local = 0;
        rr.resposta_parser = lerNome(rr.rdata, pos_local);
    } 
    
    catch (const exception& erro) {
        rr.resposta_parser = "ERRO ao decodificar NS: " + string(erro.what());
    }
}




void DNSMensagem::decodeSOA(ResourceRecords& rr) {
    
    try {
        size_t pos_local = 0;
        string mname = lerNome(rr.rdata, pos_local);
        string rname = lerNome(rr.rdata, pos_local);

    
        if (pos_local + 20 <= rr.rdata.size()) {
            uint32_t serial  = (rr.rdata[pos_local] << 24) | (rr.rdata[pos_local+1] << 16) |
                               (rr.rdata[pos_local+2] << 8) | rr.rdata[pos_local+3];
            uint32_t refresh = (rr.rdata[pos_local+4] << 24) | (rr.rdata[pos_local+5] << 16) |
                               (rr.rdata[pos_local+6] << 8) | rr.rdata[pos_local+7];
            uint32_t retry   = (rr.rdata[pos_local+8] << 24) | (rr.rdata[pos_local+9] << 16) |
                               (rr.rdata[pos_local+10] << 8) | rr.rdata[pos_local+11];
            uint32_t expire  = (rr.rdata[pos_local+12] << 24) | (rr.rdata[pos_local+13] << 16) |
                               (rr.rdata[pos_local+14] << 8) | rr.rdata[pos_local+15];
            uint32_t minimum = (rr.rdata[pos_local+16] << 24) | (rr.rdata[pos_local+17] << 16) |
                               (rr.rdata[pos_local+18] << 8) | rr.rdata[pos_local+19];

            rr.resposta_parser = "MNAME: " + mname + ", RNAME: " + rname +
                                 ", SERIAL: " + to_string(serial) +
                                 ", REFRESH: " + to_string(refresh) +
                                 ", RETRY: " + to_string(retry) +
                                 ", EXPIRE: " + to_string(expire) +
                                 ", MINIMUM: " + to_string(minimum);
        } 
        
        else {
            rr.resposta_parser = "ERRO: SOA inválido. Dados insuficientes";
        }
    } 
    
    catch (const exception& erro) {
        rr.resposta_parser = "ERRO ao decodificar SOA: " + string(erro.what());
    }

}



void DNSMensagem::decodeMX(ResourceRecords& rr) {

    if (rr.rdlen < 3) {
        rr.resposta_parser = "ERRO: MX inválido. RDLength insuficiente: " + to_string(rr.rdlen);
        return;
    }

    uint16_t preferencia = (rr.rdata[0] << 8) | rr.rdata[1]; // quanto menor o numero maior a prioridade
    size_t pos_local = 2;


    try {
    
        string destino = lerNome(rr.rdata, pos_local);
        rr.resposta_parser = "Preference: " + to_string(preferencia) + ", Exchange: " + destino;
    } 
    
    catch (const exception& erro) {
        rr.resposta_parser = "ERRO ao decodificar MX: " + string(erro.what());
    }

}




void DNSMensagem::decodeTXT(ResourceRecords& rr) {
    
    try {
        string txt;
        size_t pos_local = 0;

        while (pos_local < rr.rdata.size()) {
            uint8_t len = rr.rdata[pos_local++];
            
            if (pos_local + len > rr.rdata.size()) 
                break;

            if (!txt.empty()) 
                txt += " ";

            txt += string(rr.rdata.begin() + pos_local, rr.rdata.begin() + pos_local + len);
            
            pos_local += len;
        }

        rr.resposta_parser = txt;
    } 
    
    catch (const exception& erro) {
        rr.resposta_parser = "ERRO ao decodificar TXT: " + string(erro.what());
    }

}



void DNSMensagem::decodeOPT(ResourceRecords& rr) {

    if (rr.rdlen < 0) {
        rr.resposta_parser = "OPT RR inválido (dados insuficientes)";
        return;
    }


    edns_udp_size = rr.classe;

  
    uint32_t ttl = rr.ttl;
    uint8_t ext_rcode = (ttl >> 24) & 0xFF;
    uint8_t version   = (ttl >> 16) & 0xFF;
    uint16_t z        = ttl & 0xFFFF;

    edns_version = version;
    edns_z       = z;

    // Verifica versão EDNS
    if (version != 0) {
        cerr << "Aviso: versão EDNS não suportada (" << (int)version << "), ignorando OPT RR." << endl;
        rr.resposta_parser = "OPT RR ignorado devido à versão EDNS não suportada";
        return;
    }

    bool dnssec_ok = (z & 0x8000) != 0; 


    if (ext_rcode != 0) {
        cerr << "Erro EDNS: extended RCODE=" << (int)ext_rcode << endl;
    }

    rr.resposta_parser += "UDP size=" + to_string(edns_udp_size) + ", ";

    // Lê as opções EDNS
    size_t pos = 0;
    edns_options.clear(); 

    
    while (pos + 4 <= rr.rdata.size()) {
        
        EDNSOption opt;
        opt.code = (rr.rdata[pos] << 8) | rr.rdata[pos+1];
        uint16_t len = (rr.rdata[pos+2] << 8) | rr.rdata[pos+3];
        pos += 4;

        if (pos + len > rr.rdata.size()) {
            cerr << "Aviso: tamanho de opção EDNS maior que restante do rdata" << endl;
            break;
        }

        opt.data.assign(rr.rdata.begin() + pos, rr.rdata.begin() + pos + len);
        pos += len;

        edns_options.push_back(opt);

        rr.resposta_parser += "Opções EDNS presentes: " + to_string(edns_options.size());

    }

    if (rr.resposta_parser.empty())
        rr.resposta_parser = "OPT RR vazio ou inválido";
}



void DNSMensagem::decodeDS(ResourceRecords& rr) {
    rr.resposta_parser = "Registro DS (Delegation Signer), dados brutos: " + to_string(rr.rdlen) + " bytes";
}



void DNSMensagem::decodeRRSIG(ResourceRecords& rr) {
    rr.resposta_parser = "RRSIG RR (assinatura DNSSEC), dados brutos: " + to_string(rr.rdlen) + " bytes";
}



void DNSMensagem::decodeDNSKEY(ResourceRecords& rr) {
    rr.resposta_parser = "DNSKEY RR (chave pública), dados brutos: " + to_string(rr.rdlen) + " bytes";
}



ResourceRecords DNSMensagem::lerRegistro(const vector<uint8_t>& dados, size_t& pos) {

    ResourceRecords rr;
    
    try {
        rr.nome   = lerNome(dados, pos);
        rr.tipo   = lerUint16(dados, pos);
        rr.classe = lerUint16(dados, pos);
        rr.ttl    = lerUint32(dados, pos);
        rr.rdlen  = lerUint16(dados, pos);

        if (pos + rr.rdlen > dados.size())
            throw runtime_error("Erro: RDLength ultrapassa tamanho do pacote DNS");

        rr.rdata.assign(dados.begin() + pos, dados.begin() + pos + rr.rdlen);
        pos += rr.rdlen;

        // Decodifica conforme o tipo do RR
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
                rr.resposta_parser = "Tipo de Registro não suportado"; 
                break;
        
            }
        } 
        
        catch (const exception& erro) {

            // apenas para testes, apagar quando for enviar
            cerr << "[ERRO] Falha ao ler RR do tipo " << rr.tipo 
                << " para o nome '" << rr.nome << "' na posicao " << pos 
                << ": " << erro.what() << endl;

            rr.resposta_parser = "Registro inválido ou corrompido";
        }
    return rr;
}


void DNSMensagem::lerRespostas(const vector<uint8_t>& dados, size_t& pos, int count) {
    for (int i = 0; i < count; ++i)
        respostas.push_back(lerRegistro(dados, pos));
}


void DNSMensagem::lerAutoridade(const vector<uint8_t>& dados, size_t& pos, int count) {
    for (int i = 0; i < count; ++i)
        autoridades.push_back(lerRegistro(dados, pos));
}



void DNSMensagem::lerAdicional(const vector<uint8_t>& dados, size_t& pos, int count) {
    for (int i = 0; i < count; ++i)
        adicionais.push_back(lerRegistro(dados, pos));
}



void DNSMensagem::parseResposta(const vector<uint8_t>& dados) {
    
    if (dados.size() < 12)
        throw runtime_error("Pacote DNS inválido (muito pequeno)");

    size_t pos = 0;

    lerCabecalho(dados, pos);
    lerPergunta(dados, pos);

    lerRespostas(dados, pos, cabecalho.ancount);
    lerAutoridade(dados, pos, cabecalho.nscount);
    lerAdicional(dados, pos, cabecalho.arcount);

    // -------------------
    // Integração com Cache
    // -------------------
    // Se a cache estiver pronta, você pode adicionar algo como:
    /*
    for (const auto& rr : respostas) {
        if (rr.tipo != 0 && rr.rdlen > 0) {  // Registro válido
            CacheDaemon::addEntry(pergunta.qname, rr.resposta_parser, rr.ttl); // addentry tem que substituir pela função correta
        }
    }

    if (respostas.empty()) { // Nenhuma resposta: cache negativa
        CacheDaemon::addNegative(pergunta.qname, 60); // TTL default 60s // idem para addnegative
    }*/

}



void DNSMensagem::imprimirResposta() {

    cout << "\n================== CABEÇALHO DNS ==================\n";
    cout << "ID:        " << cabecalho.id << endl;
    cout << "Flags:     0x" << hex << setw(4) << setfill('0') << cabecalho.flags << dec << endl;
    cout << "QDCOUNT:   " << cabecalho.qdcount << endl;
    cout << "ANCOUNT:   " << cabecalho.ancount << endl;
    cout << "NSCOUNT:   " << cabecalho.nscount << endl;
    cout << "ARCOUNT:   " << cabecalho.arcount << endl;

    cout << "\n================== PERGUNTA ==================\n";
    cout << "Domínio:   " << pergunta.qname << endl;
    cout << "Tipo:      " << pergunta.qtype << endl;
    cout << "Classe:    " << pergunta.qclass << endl;

    cout << "\n================== RESPOSTAS ==================\n";
    for (const auto& rr : respostas)
        cout << rr.nome << " -> " << rr.resposta_parser << endl;

    cout << "\n================== AUTORIDADES ==================\n";
    for (const auto& rr : autoridades)
        cout << rr.nome << " -> " << rr.resposta_parser << endl;

    cout << "\n================== ADICIONAIS ==================\n";
    for (const auto& rr : adicionais)
        cout << rr.nome << " -> " << rr.resposta_parser << endl;

}
