#include "server/cache_daemon.h"
#include "cli/arguments.h"
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        Arguments args(argc, argv);

        switch (args.get_cache_command()) {
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
                args.print_summary();
                // Fluxo de resolução do DNS
                return 0;
        }
    }
    catch (const exception& e) {
        cerr << "Erro: " << e.what() << endl;
        return 1;
    }
}