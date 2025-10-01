#include "arguments.h"
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

//using namespace Arguments;

// Mapeamento
static const std::unordered_map<std::string, Mode> mode_map = {
    {"recursive", Mode::Recursive},
    {"forwarder", Mode::Forwarder},
    {"iterative", Mode::Iterative},
    {"validating", Mode::Validating},
    {"insecure", Mode::Insecure},
    {"strict-dnssec", Mode::StrictDNSSEC},
    {"dot", Mode::Dot}
};

std::string Arguments::mode_to_string(Mode m) {
    for (const auto& [key, val] : mode_map) 
        if (val == m) return key;

    return "unknown";
}

Mode Arguments::string_to_mode(const std::string& s) {
    auto it = mode_map.find(s);
    if (it != mode_map.end()) return it->second;
    return Mode::Unknown;
}

void Arguments::print_usage(const char* prog) {
    cerr << "Uso: " << prog << " --ns <server> --name <domain> --qtype <type> "
        << "[--mode <m>] [--sni <s>] [--trust-anchor <file>] "
        << "[--fanout <n>] [--workers <n>] [--timeout <sec>] [--trace]\n";
}

Arguments::Arguments(int argc, char* argv[]) {
    parse(argc, argv);
}

void Arguments::parse(int argc, char* argv[]) {
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
        {nullptr, 0, nullptr, 0}
    };

    int opt, option_index = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'n': _ns = optarg; break;
            case 'd': _name = optarg; break;
            case 'q': _qtype = optarg; break;
            case 'm': _mode = string_to_mode(optarg); break;
            case 's': _sni = string(optarg); break;
            case 't': _trustanchor_ = string(optarg); break;
            case 'f': _fanout = stoi(optarg); break;
            case 'w': _workers = stoi(optarg); break;
            case 'o': _timeout = stoi(optarg); break;
            case 'r': _trace = true; break;
            default:
                print_usage(argv[0]);
                throw invalid_argument("Argumento inválido.");
        }
    }

    // Validação
    if (_ns.empty() || _name.empty() || _qtype.empty()) {
        print_usage(argv[0]);
        throw invalid_argument("Parâmetros obrigatórios faltando!");
    }
}

void Arguments::print_summary() const {
    cout << "Configuração recebida:\n";
    cout << "NS: " << _ns << "\n";
    cout << "Nome: " << _name << "\n";
    cout << "QTYPE: " << _qtype << "\n";
    cout << "Modo: " << mode_to_string(_mode) << "\n";
    cout << "SNI: " << (_sni ? *_sni : "N/A") << "\n";
    cout << "Trust Anchor: " << (_trust_anchor ? *_trust_anchor : "N/A") << "\n";
    cout << "Fanout: " << _fanout << "\n";
    cout << "Workers: " << _workers << "\n";
    cout << "Timeout: " << _timeout << "\n";
    cout << "Trace: " << (_trace ? "ON" : "OFF") << "\n";
}