#include "server/cache_daemon.hpp"
#include "cli/arguments.hpp"
#include "dns/dns_mensagem.hpp"
#include "dns/client.hpp"
#include "dns/dot_cliente.hpp"

using namespace std;

void error_message(const string& msg, const uint16_t exit_code = 1) {
    cerr << msg << endl;
    if (exit_code)
        exit(EXIT_FAILURE);
}

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
        string valor;

        switch (args.get_cache_command()) 
        {
            case CacheCommand::Activate:
                {
                    CacheDaemon daemon;
                    daemon.run();
                }
                return 0;

            case CacheCommand::Deactivate:
                CacheDaemon::send_command("DEACTIVATE");
                return 0;
                    
            case CacheCommand::Status:
                CacheDaemon::send_command("STATUS");
                return 0;
                
            case CacheCommand::PurgeAll:
                CacheDaemon::send_command("PURGE ALL");
                return 0;
                
            case CacheCommand::PurgePositive:
                CacheDaemon::send_command("PURGE POSITIVE");
                return 0;

            case CacheCommand::PurgeNegative:
                CacheDaemon::send_command("PURGE NEGATIVE");
                return 0;

            case CacheCommand::ListAll:
                CacheDaemon::send_command("LIST ALL");
                return 0;
                
            case CacheCommand::ListPositive:
                CacheDaemon::send_command("LIST POSITIVE");
                return 0;

            case CacheCommand::ListNegative:
                CacheDaemon::send_command("LIST NEGATIVE");
                return 0;

            case CacheCommand::SetPositive:
                if (argc < 3)
                    error_message("Erro: uso correto: --set positive <tamanho>\n");
                valor = argv[2];
                CacheDaemon::send_command("SET POSITIVE " + valor);
                return 0;

            case CacheCommand::SetNegative:
                if (argc < 3)
                    error_message("Erro: uso correto: --set negative <tamanho>\n");
                valor = argv[2];
                CacheDaemon::send_command("SET NEGATIVE" + valor);
                return 0;
            case CacheCommand::CachePut:
                if (argc < 5)
                    error_message("Erro: uso correto: --cache put <nome> <valor> <ttl>\n");
                {
                    string nome = argv[2];
                    string valor_ip = argv[3];
                    string ttl = argv[4];
                    CacheDaemon::send_command("CACHE PUT " + nome + " " + valor_ip + " " + ttl);
                }
                return 0;

            case CacheCommand::CacheGet:
                if (argc < 3)
                    error_message("Erro: uso correto: --cache get <nome>\n");
                {
                    string nome = argv[2];
                    CacheDaemon::send_command("CACHE GET " + nome);
                }
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
                bool sucesso = false;
                
                try {
                    DNSClient client;         
                    vector<uint8_t> resultado_bytes = client.resolvedor(args.get_name(), qtype);

                    if (!resultado_bytes.empty()) {
                        resposta_final.parseResposta(resultado_bytes);
                        sucesso = true;

                        // Verifica se houve truncamento (TC=1)
                        if (resposta_final.cabecalho.flags & 0x0200) {
                            cout << "Resposta UDP truncada (TC=1), tentando TCP..." << endl;
                            sucesso = false;
                        }
                    }
                } catch (const exception& e_udp) {
                    cerr << "UDP falhou: " << e_udp.what() << endl;
                }
                
                if (!sucesso) {
                    try {
                        DOTCliente dotClient(args.get_ns(), 853);
                        if (dotClient.conectar()) {
                            resposta_final.configurarConsulta(args.get_name(), qtype);
                            if (dotClient.enviarQuery(resposta_final) && dotClient.receberResposta(resposta_final))
                                sucesso = true;
                        }
                    } catch (const exception& e_dot) {
                        cerr << "DoT falhou: " << e_dot.what() << endl;
                    }
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
