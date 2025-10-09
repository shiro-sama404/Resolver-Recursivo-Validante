#include "server/cache_daemon.h"
#include "cli/arguments.h"
#include "dns/dns_mensagem.h" 
#include "dns/client.h"     
#include <iostream>
#include <unordered_map>     

using namespace std;

uint16_t qtype_to_uint16(const string& qtype_str) //Para traduzir a string
{
    static const unordered_map<string, uint16_t> qtype_map = { {"A", 1}, {"NS", 2}, {"CNAME", 5}, {"SOA", 6}, {"MX", 15}, {"TXT", 16}, {"AAAA", 28} };

    auto it = qtype_map.find(qtype_str);

    if (it != qtype_map.end()) 
        return it->second;
    
    cerr << "Aviso: nao foi possivel reconhecer QTYPE '" << qtype_str << "'. Usando 'A' como padrão.\n" << endl;
    
    return 1;
}


int main(int argc, char* argv[]) 
{
    try {
        Arguments args(argc, argv);

        switch (args.get_cache_command()) 
        {
            case CacheCommand::Activate:
                cout << "Iniciando Cache Daemon..." << endl;
                {
                    CacheDaemon daemon;
                    daemon.run();
                }
                return 0;

            case CacheCommand::Status:
                CacheDaemon::send_command("STATUS");
                return 0;

            case CacheCommand::PurgeAll:
                CacheDaemon::send_command("PURGE_ALL");
                return 0;

            case CacheCommand::Deactivate:
                CacheDaemon::send_command("DEACTIVATE");
                return 0;

            default:
            
                // Fluxo de resolução do DNS
                
                
                if (args.get_name().empty() || args.get_qtype().empty()) 
                 {
                cerr << "Erro: '--name' e '--qtype' sao argumentos obrigatorios." << endl;
                Arguments::print_usage(argv[0]);
                return 1;
                }

                args.print_summary();
                
                try {
                    DNSClient client;
                    uint16_t qtype = qtype_to_uint16(args.get_qtype());
                    
                    vector<uint8_t> resultado_bytes = client.resolve(args.get_name(), qtype);
                    
                    if (!resultado_bytes.empty()) 
                    {
                        cout << "\n\n    RESPOSTA FINAL DO RESOLVEDOR" << endl;
                        cout << "-----------------------------------\n" << endl;
                        
                        DNSMensagem msg_final;
                        msg_final.parseResposta(resultado_bytes);
                        msg_final.imprimirResposta(); 

                    } else 
                        cout << "\n\nNao encontrou resposta para " << args.get_name() << endl;
            
                    } catch (const exception& e) 
                    {
                        cerr << "\nErro durante a resolucao: " << e.what() << endl;
                        return 1;
                    }
            
               return 0;
        }    
    }
    catch (const exception& e) {
        cerr << "Erro: " << e.what() << endl;
        return 1;
    }
}
