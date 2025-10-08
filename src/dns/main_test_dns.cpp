#include <iostream>
#include "dns_mensagem.h"

int main() {
    DNSMensagem dns;

    // Preenche a pergunta de teste
    dns.pergunta.qname = "example.com";
    dns.pergunta.qtype = 1;  // Tipo A
    dns.pergunta.qclass = 1; // IN

    // Monta o pacote
    std::vector<uint8_t> pacote = dns.montarQuery();

    dns.configurarConsulta("example.com", 1);


    // Mostra resultado
    std::cout << "Pacote montado com " << pacote.size() << " bytes.\n";
    std::cout << "Primeiros 20 bytes:\n";
    for (size_t i = 0; i < pacote.size() && i < 20; ++i) {
        std::cout << std::hex << (int)pacote[i] << " ";
    }
    std::cout << std::dec << std::endl;

    DNSMensagem msg;


    return 0;
}
