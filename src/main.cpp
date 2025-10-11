#include "server/cache_daemon.hpp"
#include "cli/arguments.hpp"
#include "dns/dns_mensagem.hpp"
#include "dns/client.hpp"
#include "dns/dot_cliente.hpp"

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
                
                uint16_t qtype = qtype_to_uint16(args.get_qtype());
                DNSMensagem resposta_final;

                bool dot_sucesso = false;
                
                try
                {
                    DOTCliente dotClient(args.get_ns(), 853);
                    if (dotClient.conectar())
                    {
                        resposta_final.configurarConsulta(args.get_name(), qtype);
                       
                        if (dotClient.enviarQuery(resposta_final) && dotClient.receberResposta(resposta_final))
                            dot_sucesso = true;
                    }
                }
                
                catch (const exception& e_dot)
                {
                    cerr << "DoT falhou, fallback para DNSClient: " << e_dot.what() << endl;
                }
                
                if (!dot_sucesso)
                {
                    DNSClient client;         
                    vector<uint8_t> resultado_bytes = client.resolvedor(args.get_name(), qtype);

                    if (!resultado_bytes.empty()) 
                        resposta_final.parseResposta(resultado_bytes);
                }
                if (!resposta_final.respostas.empty() || !resposta_final.autoridades.empty() || !resposta_final.adicionais.empty()) 
                {
                    cout << "\n\n    RESPOSTA FINAL DO RESOLVEDOR" << endl;
                    cout << "-----------------------------------\n" << endl;
                    resposta_final.imprimirResposta();

                } else
                    cout << "\n\nNao encontrou resposta para " << args.get_name() << endl;
            
                return 0;
        }    
    }
    catch (const exception& e) {
        cerr << "Erro: " << e.what() << endl;
        return 1;
    }
}