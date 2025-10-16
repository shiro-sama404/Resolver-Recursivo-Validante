#include "arguments.hpp"

#include <iomanip>

using namespace std;

// Mapeamento estático para conversão de string em Mode
static const unordered_map<string, Mode> mode_map = {
    {"recursive", Mode::Recursive}, {"forwarder", Mode::Forwarder},
    {"iterative", Mode::Iterative}, {"validating", Mode::Validating},
    {"insecure", Mode::Insecure},   {"strict-dnssec", Mode::StrictDNSSEC},
    {"dot", Mode::Dot}
};

Arguments::Arguments(int argc, char* argv[])
{
    try
    {
        parse(argc, argv);
    }
    catch(const exception& e)
    {
        cerr << "Erro: " << e.what() << endl;
        printUsage();
    }
}

string Arguments::modeToString(Mode m)
{
    for (const auto& [key, val] : mode_map)
        if (val == m)
            return key;
    return "unknown";
}

Mode Arguments::stringToMode(const string& s)
{
    auto it = mode_map.find(s);
    if (it != mode_map.end())
        return it->second;
    return Mode::Unknown;
}

void Arguments::printUsage()
{
    cerr << R"(
                 ┌──────────────────────────────────────────┐
                 │  DNSResolver - Resolvedor DNS Recursivo  │
                 └──────────────────────────────────────────┘

    Uma ferramenta para realizar consultas DNS e gerenciar um daemon de cache.

    ================================================================================
    Uso Principal:
    ================================================================================

    ▶ Para consultas DNS:
        ./resolver --name <domínio> [opções de consulta...]

    ▶ Para gerenciar o cache:
        ./resolver <comando de cache> [argumentos...]

    ================================================================================
    Opções de Consulta DNS 🔍
    ================================================================================

    --name  <domínio>        (Obrigatório) Domínio a ser resolvido (ex: google.com).
    --qtype <tipo>           Tipo de registro (A, AAAA, MX, NS, SOA). Padrão: A.
    --ns    <servidor>       Endereço IP do servidor DNS a ser consultado. Padrão: 8.8.8.8.
    --mode  <modo>           Modo de operação: recursive, iterative, dot. Padrão: recursive.
    --trace                  Ativa o rastreamento detalhado da resolução, exibindo cada passo.
    --timeout <segundos>     Tempo máximo de espera por uma resposta. Padrão: 5.

    Opções de DoT:
    --sni   <hostname>       Define o Server Name Indication (SNI) para a conexão DoT.

    ================================================================================
    Opções de Gerenciamento da Cache 📦
    ================================================================================

    Controle do Daemon:
    --activate               Inicia o daemon de cache em segundo plano.
    --deactivate             Encerra o daemon de cache.

    Inspeção:
    --status                 Exibe o status atual das caches (uso/capacidade).
    --list-positive          Lista todas as entradas da cache positiva.
    --list-negative          Lista todas as entradas da cache negativa.
    --list-all               Lista todas as entradas de ambas as caches.

    Modificação:
    --purge-positive         Limpa (expurga) a cache positiva.
    --purge-negative         Limpa a cache negativa.
    --purge-all              Limpa ambas as caches.
    --set-positive <tam>     Define o tamanho máximo da cache positiva.
    --set-negative <tam>     Define o tamanho máximo da cache negativa.

    ================================================================================
    Exemplos de Uso:
    ================================================================================

    # Consulta A para ufms.br
    ./resolver --name ufms.br --qtype A

    # Consulta MX para gmail.com com trace ativado
    ./resolver --name gmail.com --qtype MX --trace

    # Consulta iterativa para github.com, começando por um servidor raiz
    ./resolver --name github.com --qtype A --mode iterative --ns 198.41.0.4

    # Ativar o daemon de cache
    ./resolver --activate

    # Verificar o status da cache
    ./resolver --status

    )" << endl;
}

void Arguments::parse(int argc, char* argv[])
{
    const option long_options[] = {
        {"ns",           required_argument, nullptr, 'n'}, {"name",    required_argument, nullptr, 'd'},
        {"qtype",        required_argument, nullptr, 'q'}, {"mode",    required_argument, nullptr, 'm'},
        {"sni",          required_argument, nullptr, 's'}, {"trust-anchor", required_argument, nullptr, 't'},
        {"fanout",       required_argument, nullptr, 'f'}, {"workers", required_argument, nullptr, 'w'},
        {"timeout",      required_argument, nullptr, 'o'}, {"trace",   no_argument,       nullptr, 'r'},
        {"activate",     no_argument, nullptr, 1000},      {"deactivate",   no_argument, nullptr, 1001},
        {"status",       no_argument, nullptr, 1002},      {"purge-positive", no_argument, nullptr, 1003},
        {"purge-negative", no_argument, nullptr, 1004},    {"purge-all",    no_argument, nullptr, 1005},
        {"list-positive",  no_argument, nullptr, 1006},    {"list-negative",no_argument, nullptr, 1007},
        {"list-all",     no_argument, nullptr, 1008},      {"set-positive", required_argument, nullptr, 1009},
        {"set-negative", required_argument, nullptr, 1010},{"put",          required_argument, nullptr, 1011},
        {"put-negative", required_argument, nullptr, 1012},{"get",          required_argument, nullptr, 1013},
        {"get-negative", required_argument, nullptr, 1014},{nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "n:d:q:m:s:t:f:w:o:r", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'n': _ns = optarg; _has_dns_args = true; break;
            case 'd': _name = optarg; _has_dns_args = true; break;
            case 'q': _qtype = optarg; _has_dns_args = true; break;
            case 'm': _mode = stringToMode(optarg); _has_dns_args = true; break;
            case 's': _sni = string(optarg); _has_dns_args = true; break;
            case 't': _trust_anchor = string(optarg); _has_dns_args = true; break;
            case 'f': _fanout = stoi(optarg); _has_dns_args = true; break;
            case 'w': _workers = stoi(optarg); _has_dns_args = true; break;
            case 'o': _timeout = stoi(optarg); _has_dns_args = true; break;
            case 'r': _trace = true; _has_dns_args = true; break;

            case 1000: _cache_command = CacheCommand::Activate; break;
            case 1001: _cache_command = CacheCommand::Deactivate; break;
            case 1002: _cache_command = CacheCommand::Status; break;
            case 1003: _cache_command = CacheCommand::PurgePositive; break;
            case 1004: _cache_command = CacheCommand::PurgeNegative; break;
            case 1005: _cache_command = CacheCommand::PurgeAll; break;
            case 1006: _cache_command = CacheCommand::ListPositive; break;
            case 1007: _cache_command = CacheCommand::ListNegative; break;
            case 1008: _cache_command = CacheCommand::ListAll; break;
            case 1009: _cache_command = CacheCommand::SetPositive; break;
            case 1010: _cache_command = CacheCommand::SetNegative; break;
            case 1011: _cache_command = CacheCommand::Put; break;
            case 1012: _cache_command = CacheCommand::PutNegative; break;
            case 1013: _cache_command = CacheCommand::Get; break;
            case 1014: _cache_command = CacheCommand::GetNegative; break;

            case '?': throw invalid_argument("Argumento inválido.");
        }
    }
}

void Arguments::printSummary() const
{
    
    // Códigos de cores ANSI
    const string COLOR_HEADER = "\033[1;34m"; // Azul Negrito
    const string COLOR_LABEL  = "\033[1;37m"; // Branco Negrito
    const string COLOR_VALUE  = "\033[0;33m"; // Amarelo
    const string COLOR_RESET  = "\033[0m";

    const int label_width = 20; // Largura da coluna de rótulos

    cout << endl;
    cout << COLOR_HEADER << "┌──────────────────────────────────────────────┐" << COLOR_RESET << endl;
    cout << COLOR_HEADER << "│          Resumo da Configuração DNS          │" << COLOR_RESET << endl;
    cout << COLOR_HEADER << "└──────────────────────────────────────────────┘" << COLOR_RESET << endl;

    cout << COLOR_LABEL << left << setw(label_width) << "  Domínio: " << COLOR_VALUE 
              << getName() << COLOR_RESET << endl;

    cout << COLOR_LABEL << left << setw(label_width) << "  Tipo de Registro:" << COLOR_VALUE 
              << getQtype() << COLOR_RESET << endl;

    cout << COLOR_LABEL << left << setw(label_width) << "  Servidor NS:" << COLOR_VALUE 
              << getNs() << COLOR_RESET << endl;
              
    cout << COLOR_LABEL << left << setw(label_width) << "  Modo de Operação: " << COLOR_VALUE 
              << modeToString(getMode()) << COLOR_RESET << endl;

    cout << COLOR_LABEL << left << setw(label_width) << "  Timeout:" << COLOR_VALUE 
              << getTimeout() << "s" << COLOR_RESET << endl;

    cout << COLOR_LABEL << left << setw(label_width) << "  Trace:" << COLOR_VALUE 
              << (isTraceEnabled() ? "ATIVADO" : "DESATIVADO") << COLOR_RESET << endl;

    if (getSni().has_value())
        cout << COLOR_LABEL << left << setw(label_width) << "  SNI (DoT):" << COLOR_VALUE 
                  << getSni().value() << COLOR_RESET << endl;
    if (getTrustAnchor().has_value())
        cout << COLOR_LABEL << left << setw(label_width) << "  Âncora DNSSEC:" << COLOR_VALUE 
                  << getTrustAnchor().value() << COLOR_RESET << endl;

    cout << endl;
}