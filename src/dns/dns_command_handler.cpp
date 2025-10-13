#include "dns_command_handler.hpp"

int DNSCommandHandler::execute(const Arguments& args, int argc, char* argv[])
{
    try
    {

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
    catch (const std::exception& e) {
        cerr << "Erro: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}