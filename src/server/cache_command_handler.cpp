#include "cache_command_handler.hpp"

#include <iostream>
#include <sstream>

#include "../server/cache_client.hpp"
#include "../utils/consoleUtils.hpp"

using namespace std;

int CacheCommandHandler::execute(const Arguments& args, int argc, char* argv[])
{
    try
    {
        CacheCommand command = args.getCacheCommand();
        stringstream command_stream;

        if (command == CacheCommand::Activate)
        {
            CacheStore store;
            CacheController controller(store);
            CacheServer server(controller);
            server.run();
            return EXIT_SUCCESS;
        }

        if (!CacheClient::isCacheActive)
        {
            cerr << "Cache Daemon inativo." << endl;
            args.printUsage();
            return EXIT_FAILURE;
        }

        switch (command)
        {
            case CacheCommand::Deactivate:
                command_stream << "SHUTDOWN";
                break;
            case CacheCommand::Status:
                command_stream << "STATUS";
                break;
            case CacheCommand::PurgeAll:
                command_stream << "PURGE ALL";
                break;
            case CacheCommand::PurgePositive:
                command_stream << "PURGE POSITIVE";
                break;
            case CacheCommand::PurgeNegative:
                command_stream << "PURGE NEGATIVE";
                break;
            case CacheCommand::ListAll:
                command_stream << "LIST ALL";
                break;
            case CacheCommand::ListPositive:
                command_stream << "LIST POSITIVE";
                break;
            case CacheCommand::ListNegative:
                command_stream << "LIST NEGATIVE";
                break;
            case CacheCommand::SetPositive:
            {
                if (argc < 3) exitErrorMessage("Uso: --set-positive <tamanho>");
                command_stream << "SET POSITIVE " << argv[2];
                break;
            }
            case CacheCommand::SetNegative:
            {
                if (argc < 3) exitErrorMessage("Uso: --set-negative <tamanho>");
                command_stream << "SET NEGATIVE " << argv[2];
                break;
            }
            case CacheCommand::Get:
            {
                if (argc < 3) exitErrorMessage("Uso: --get <qname> [qtype] [qclass]");
                string qname = argv[2];
                string qtype = (argc > 3) ? argv[3] : "1";   // Padrão A
                string qclass = (argc > 4) ? argv[4] : "1"; // Padrão IN
                command_stream << "GET " << qname << " " << qtype << " " << qclass;
                break;
            }
            case CacheCommand::GetNegative:
            {
                 if (argc < 3) exitErrorMessage("Uso: --get-negative <qname> [qtype] [qclass]");
                string qname = argv[2];
                string qtype = (argc > 3) ? argv[3] : "1";
                string qclass = (argc > 4) ? argv[4] : "1";
                command_stream << "GET_NEGATIVE " << qname << " " << qtype << " " << qclass;
                break;
            }
            case CacheCommand::Put:
            {
                 if (argc < 7) exitErrorMessage("Uso: --put <qname> <qtype> <qclass> <rdata> <ttl> <validated>");
                 command_stream << "PUT " << argv[2] << " " << argv[3] << " " << argv[4] << " " << argv[5] << " " << argv[6] << " " << argv[7];
                 break;
            }
            case CacheCommand::PutNegative:
            {
                 if (argc < 6) exitErrorMessage("Uso: --put-negative <qname> <qtype> <qclass> <reason> <ttl>");
                 command_stream << "PUT_NEGATIVE " << argv[2] << " " << argv[3] << " " << argv[4] << " " << argv[5] << " " << argv[6];
                 break;
            }
            default:
                cerr << "Comando de cache inválido ou não suportado.\n";
                return EXIT_FAILURE;
        }

        cout << CacheClient::sendCommand(command_stream.str()) << endl;
        return EXIT_SUCCESS;
    }
    catch (const exception& e)
    {
        cerr << "Erro: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}