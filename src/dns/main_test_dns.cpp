/*#include <iostream>
#include "dns_mensagem.h"

int main() {
    DNSMensagem dns;

    // Configura a consulta
    dns.configurarConsulta("example.com", 1); // Tipo A

    // Monta o pacote
    std::vector<uint8_t> pacote = dns.montarQuery();

    // Mostra resultado
    std::cout << "Pacote montado com " << pacote.size() << " bytes.\n";
    std::cout << "Primeiros 20 bytes:\n";
    for (size_t i = 0; i < pacote.size() && i < 20; ++i) {
        std::cout << std::hex << (int)pacote[i] << " ";
    }
    std::cout << std::dec << std::endl;

    std::cout << "Teste concluído.\n";
    return 0;
}*/

/* main_test_dns.cpp
 * Teste offline para DNSMensagem (sem uso de sockets)
 * Autor: Fernanda
 */
/* 
 * main_test_dns.cpp
 * Teste offline para dns_mensagem.h / dns_mensagem.cpp
 * Autor: Fernanda Neves
 */






 /* 
 
 ///CASO DE TESTE QUE FUNCIONA 
#include <iostream>
#include <iomanip>
#include <vector>
#include "dns_mensagem.h"

using namespace std;

// Função auxiliar para imprimir vetor em formato hexadecimal
void imprimirHex(const vector<uint8_t>& dados) {
    cout << hex << setfill('0');
    for (size_t i = 0; i < dados.size(); ++i) {
        cout << setw(2) << (int)dados[i] << " ";
        if ((i + 1) % 16 == 0)
            cout << endl;
    }
    cout << dec << endl;
}

int main() {
    try {
        cout << "\n===== TESTE OFFLINE DNSMensagem =====\n";

        DNSMensagem dns;
        string dominio = "example.com";

        cout << "\n[1] Configurando consulta para " << dominio << "...\n";
        dns.configurarConsulta(dominio, 1); // Tipo A

        cout << "[2] Montando pacote de query...\n";
        vector<uint8_t> query = dns.montarQuery();

        cout << "Pacote montado com " << query.size() << " bytes.\n";
        cout << "Conteúdo (hex):\n";
        imprimirHex(query);

        // Simula resposta DNS real (simplificada)
        // Essa resposta representa o domínio "example.com" com IP 93.184.216.34
        vector<uint8_t> resposta = {
            0x12, 0x34, // ID
            0x81, 0x80, // Flags: resposta padrão sem erro
            0x00, 0x01, // QDCOUNT = 1
            0x00, 0x01, // ANCOUNT = 1
            0x00, 0x00, // NSCOUNT = 0
            0x00, 0x00, // ARCOUNT = 0
            // Pergunta
            0x07, 'e','x','a','m','p','l','e',
            0x03, 'c','o','m', 0x00,
            0x00, 0x01, // Tipo A
            0x00, 0x01, // Classe IN
            // Resposta
            0xC0, 0x0C, // Ponteiro pro nome (offset 12)
            0x00, 0x01, // Tipo A
            0x00, 0x01, // Classe IN
            0x00, 0x00, 0x00, 0x3C, // TTL = 60s
            0x00, 0x04, // Tamanho = 4 bytes
            0x5D, 0xB8, 0xD8, 0x22  // IP 93.184.216.34
        };

        cout << "\n[3] Testando parseResposta() com resposta simulada...\n";
        DNSMensagem respostaDNS;
        respostaDNS.parseResposta(resposta);

        cout << "\n[4] Exibindo resultado do parsing:\n";
        respostaDNS.imprimirResposta();

        cout << "\n===== TESTE CONCLUÍDO COM SUCESSO =====\n";
    } 
    catch (const exception& e) {
        cerr << "Erro durante o teste: " << e.what() << endl;
    }

    return 0;
}
    */
   
   
   
   
   
#include <iostream>
#include <vector>
#include "dns_mensagem.h"

using namespace std;

int main() {
    cout << "\n===== TESTE OFFLINE DNSMensagem (CASO QUE FALHA) =====\n";

    DNSMensagem dns;

    // Pacote truncado: apenas 5 bytes (menor que 12, cabeçalho incompleto)
    vector<uint8_t> pacote_invalido = {0x12, 0x34, 0x01, 0x00, 0x00};

    try {
        cout << "Tentando parse de pacote truncado...\n";
        dns.parseResposta(pacote_invalido);
        cout << "Parse concluído (deveria ter falhado)!\n";
    } catch (const exception& e) {
        cerr << "Exceção capturada: " << e.what() << endl;
    }

    // Pacote truncado durante o nome
    vector<uint8_t> pacote_nome_truncado = {
        0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x07, 'e','x','a' // nome incompleto
    };

    try {
        cout << "\nTentando parse de pacote com nome truncado...\n";
        dns.parseResposta(pacote_nome_truncado);
        cout << "Parse concluído (deveria ter falhado)!\n";
    } catch (const exception& e) {
        cerr << "Exceção capturada: " << e.what() << endl;
    }

    cout << "\n===== TESTE DE ERROS CONCLUÍDO =====\n";
    return 0;
}







