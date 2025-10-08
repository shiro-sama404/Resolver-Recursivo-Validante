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