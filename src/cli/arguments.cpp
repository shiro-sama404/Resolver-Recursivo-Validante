#include "arguments.hpp"

// Mapeamento
static const unordered_map<string, Mode> mode_map = {
    {"recursive", Mode::Recursive},
    {"forwarder", Mode::Forwarder},
    {"iterative", Mode::Iterative},
    {"validating", Mode::Validating},
    {"insecure", Mode::Insecure},
    {"strict-dnssec", Mode::StrictDNSSEC},
    {"dot", Mode::Dot}
};

Arguments::Arguments(int argc, char* argv[]) {parse(argc, argv);}

string Arguments::mode_to_string(Mode m)
{
    for (const auto& [key, val] : mode_map) 
        if (val == m) return key;
    return "unknown";
}

Mode Arguments::string_to_mode(const string& s)
{
    auto it = mode_map.find(s);
    if (it != mode_map.end()) return it->second;
    return Mode::Unknown;
}

void Arguments::print_usage(const char* prog)
{
    cerr << "Uso: " << prog << endl
         << "Comandos de DNS:" << endl
         << "[--ns <server>] [--name <domain>] [--qtype <type>] [--mode <m>]" << endl
         << "[--sni <s>] [--trust-anchor <file>] [--fanout <n>] [--workers <n>]" << endl
         << "[--timeout <sec>] [--trace]" << endl
         << "Comandos de Cache:" << endl
         << "[--activate | --deactivate | --status]" << endl
         << "[--purge-positive | --purge-negative | --purge-all]" << endl
         << "[--list-positive | --list-negative | --list-all]" << endl
         << "[--set-positive <n> | --set-negative <n>]" << endl
         << "[--cache-put <n> <v> <t> | --cache-get <n> <v> <t>]" << endl;
}

void Arguments::parse(int argc, char* argv[])
{
    constexpr option long_options[] = {
        {"ns", required_argument, nullptr, 'n'},
        {"name", required_argument, nullptr, 'd'},
        {"qtype", required_argument, nullptr, 'q'},
        {"mode", required_argument, nullptr, 'm'},
        {"sni", required_argument, nullptr, 's'},
        {"trust-anchor", required_argument, nullptr, 't'},
        {"fanout", required_argument, nullptr, 'f'},
        {"workers", required_argument, nullptr, 'w'},
        {"timeout", required_argument, nullptr, 'o'},
        {"trace", no_argument, nullptr, 'r'},

        // comandos da cache
        {"activate", no_argument, nullptr, 1000},
        {"deactivate", no_argument, nullptr, 1001},
        {"status", no_argument, nullptr, 1002},
        {"purge-positive", no_argument, nullptr, 1003},
        {"purge-negative", no_argument, nullptr, 1004},
        {"purge-all", no_argument, nullptr, 1005},
        {"list-positive", no_argument, nullptr, 1006},
        {"list-negative", no_argument, nullptr, 1007},
        {"list-all", no_argument, nullptr, 1008},
        {"set-positive", required_argument, nullptr, 1009},
        {"set-negative", required_argument, nullptr, 1010},
        {"cache-put", required_argument, nullptr, 1011},
        {"cache-get", required_argument, nullptr, 1012},

        {nullptr, 0, nullptr, 0}
    };

    int opt, option_index = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            // parâmetros DNS
            case 'n': _ns = optarg; break;
            case 'd': _name = optarg; break;
            case 'q': _qtype = optarg; break;
            case 'm': _mode = string_to_mode(optarg); break;
            case 's': _sni = string(optarg); break;
            case 't': _trust_anchor = string(optarg); break;
            case 'f': _fanout = stoi(optarg); break;
            case 'w': _workers = stoi(optarg); break;
            case 'o': _timeout = stoi(optarg); break;
            case 'r': _trace = true; break;

            // comandos da cache
            case 1000: _cacheCommand = CacheCommand::Activate; break;
            case 1001: _cacheCommand = CacheCommand::Deactivate; break;
            case 1002: _cacheCommand = CacheCommand::Status; break;
            case 1003: _cacheCommand = CacheCommand::PurgePositive; break;
            case 1004: _cacheCommand = CacheCommand::PurgeNegative; break;
            case 1005: _cacheCommand = CacheCommand::PurgeAll; break;
            case 1006: _cacheCommand = CacheCommand::ListPositive; break;
            case 1007: _cacheCommand = CacheCommand::ListNegative; break;
            case 1008: _cacheCommand = CacheCommand::ListAll; break;
            case 1009: _cacheCommand = CacheCommand::SetPositive; break;
            case 1010: _cacheCommand = CacheCommand::SetNegative; break;
            case 1011: _cacheCommand = CacheCommand::CachePut; break;
            case 1012: _cacheCommand = CacheCommand::CacheGet; break;
            default:
                print_usage(argv[0]);
                throw invalid_argument("Argumento inválido.");
        }
    }
}

void Arguments::print_summary() const
{
    cout << "Configuração recebida:" << endl;
    cout << "NS: " << _ns << endl;
    cout << "Nome: " << _name << endl;
    cout << "QTYPE: " << _qtype << endl;
    cout << "Modo: " << mode_to_string(_mode) << endl;
    cout << "SNI: " << (_sni ? *_sni : "N/A") << endl;
    cout << "Trust Anchor: " << (_trust_anchor ? *_trust_anchor : "N/A") << endl;
    cout << "Fanout: " << _fanout << endl;
    cout << "Workers: " << _workers << endl;
    cout << "Timeout: " << _timeout << endl;
    cout << "Trace: " << (_trace ? "ON" : "OFF") << endl;
}