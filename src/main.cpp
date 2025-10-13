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

    Arguments::print_usage(argv[0]);
    return EXIT_FAILURE;
}