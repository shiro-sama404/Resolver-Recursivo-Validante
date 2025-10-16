#include "server/cache_command_handler.hpp"
#include "dns/dns_command_handler.hpp"

int main(int argc, char* argv[]) 
{
    Arguments args(argc, argv);

    if (args.getCacheCommand() != CacheCommand::None)
        return CacheCommandHandler::execute(args, argc, argv);

    if (args.hasDnsArgs())
    {
        DNSClient client;
        DNSCommandHandler handler(args, client);
        return handler.execute();
    }
    
    return EXIT_FAILURE;
}
