#include "cli/arguments.hpp"
#include "server/cache_command_handler.hpp"
#include "dns/dns_command_handler.hpp"

int main(int argc, char* argv[]) 
{

    Arguments args(argc, argv);

    if (args.get_cache_command() != CacheCommand::None)
        return CacheCommandHandler::execute(args, argc, argv);
    
    if (args.has_dns_args())
        return DNSCommandHandler::execute(args, argc, argv);

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
                       string servidor_dot;

                        if (args.get_ns().empty()) {
                            servidor_dot = "dns-google.com";
                        } else {
                            servidor_dot = args.get_ns();
                        }

                        DOTCliente dotCliente(servidor_dot, 853);

                        if (dotCliente.conectar()) {
                            resposta_final.configurarConsulta(args.get_name(), qtype);
                            if (dotCliente.enviarQuery(resposta_final) && dotCliente.receberResposta(resposta_final))
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
    Arguments::print_usage(argv[0]);
    return EXIT_FAILURE;
}
