#include "cache_command_handler.hpp"

int CacheCommandHandler::execute(const Arguments& args, int argc, char* argv[])
{
    try
    {
        auto cmd = args.get_cache_command();

        switch (cmd)
        {
            case CacheCommand::Activate:
                {
                    CacheStore store;
                    CacheController controller(store);
                    CacheServer server(controller);
                    server.run();
                }
                return EXIT_SUCCESS;

            case CacheCommand::Deactivate:
                cout << CacheClient::send_command("SHUTDOWN") << endl;
                return EXIT_SUCCESS;
                
            case CacheCommand::Status:
                cout << CacheClient::send_command("STATUS") << endl;
                return EXIT_SUCCESS;
                
            case CacheCommand::PurgeAll:
                cout << CacheClient::send_command("PURGE ALL") << endl;
                return EXIT_SUCCESS;
                
            case CacheCommand::PurgePositive:
                cout << CacheClient::send_command("PURGE POSITIVE") << endl;
                return EXIT_SUCCESS;

            case CacheCommand::PurgeNegative:
                cout << CacheClient::send_command("PURGE NEGATIVE") << endl;
                return EXIT_SUCCESS;

            case CacheCommand::ListAll:
                cout << CacheClient::send_command("LIST ALL") << endl;
                return EXIT_SUCCESS;
                
            case CacheCommand::ListPositive:
                cout << CacheClient::send_command("LIST POSITIVE") << endl;
                return EXIT_SUCCESS;

            case CacheCommand::ListNegative:
                cout << CacheClient::send_command("LIST NEGATIVE") << endl;
                return EXIT_SUCCESS;

            case CacheCommand::SetPositive:
                if (argc < 3)
                    exit_error_message("Erro: uso correto: --set positive <tamanho>\n");
                {
                    string valor = argv[2];
                    cout << CacheClient::send_command("SET POSITIVE " + valor) << endl;
                }
                return EXIT_SUCCESS;

            case CacheCommand::SetNegative:
                if (argc < 3)
                    exit_error_message("Erro: uso correto: --set negative <tamanho>\n");
                {
                    string valor = argv[2];
                    cout << CacheClient::send_command("SET NEGATIVE " + valor) << endl;
                }
                
                return EXIT_SUCCESS;
            case CacheCommand::CachePut:
                if (argc < 5)
                    exit_error_message("Erro: uso correto: --cache-put <nome> <valor> <ttl>\n");
                {
                    string nome = argv[2];
                    string valor_ip = argv[3];
                    string ttl = argv[4];
                    cout << CacheClient::send_command("CACHE_PUT " + nome + " " + valor_ip + " " + ttl) << endl;
                }
                return EXIT_SUCCESS;

            case CacheCommand::CachePutNegative:
                if (argc < 5)
                    exit_error_message("Erro: uso correto: --cache-put-negative <nome> <valor> <ttl>\n");
                {
                    string nome = argv[2];
                    string valor_ip = argv[3];
                    string ttl = argv[4];
                    cout << CacheClient::send_command("CACHE_PUT_NEGATIVE " + nome + " " + valor_ip + " " + ttl) << endl;
                }
                return EXIT_SUCCESS;
            
            case CacheCommand::CacheGet:
                if (argc < 3)
                    exit_error_message("Erro: uso correto: --cache-get <nome>\n");
                {
                    string nome = argv[2];
                    cout << CacheClient::send_command("CACHE_GET " + nome) << endl;
                }
                return EXIT_SUCCESS;

            case CacheCommand::CacheGetNegative:
                if (argc < 3)
                    exit_error_message("Erro: uso correto: --cache-get-negative <nome>\n");
                {
                    string nome = argv[2];
                    cout << CacheClient::send_command("CACHE_GET_NEGATIVE " + nome) << endl;
                }
                return EXIT_SUCCESS;
            default:
                cerr << "Comando de cache inválido ou não suportado.\n";
                return EXIT_FAILURE;
        }
    }
    catch (const exception& e)
    {
        cerr << "Erro: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}