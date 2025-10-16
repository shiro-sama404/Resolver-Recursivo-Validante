#pragma once

#include <getopt.h>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Modos de operação do resolvedor DNS.
 */
enum class Mode
{
    Recursive,
    Forwarder,
    Iterative,
    Validating,
    Insecure,
    StrictDNSSEC,
    Dot,
    Unknown
};

/**
 * @brief Comandos específicos para o gerenciamento do cache via linha de comando.
 */
enum class CacheCommand
{
    None,
    Activate,
    Deactivate,
    Status,
    PurgePositive,
    PurgeNegative,
    PurgeAll,
    ListPositive,
    ListNegative,
    ListAll,
    SetPositive,
    SetNegative,
    Put,
    PutNegative,
    Get,
    GetNegative
};

/**
 * @brief Analisa, armazena e fornece acesso aos argumentos da linha de comando.
 */
class Arguments
{
public:
    /**
     * @brief Constrói e analisa os argumentos da linha de comando.
     * @param argc O número de argumentos.
     * @param argv O vetor de argumentos.
     */
    Arguments(int argc, char* argv[]);

public:
     // --- Getters ---
    const std::string& getNs() const { return _ns; }
    const std::string& getName() const { return _name; }
    const std::string& getQtype() const { return _qtype; }
    Mode getMode() const { return _mode; }
    const std::optional<std::string>& getSni() const { return _sni; }
    const std::optional<std::string>& getTrustAnchor() const { return _trust_anchor; }
    int getFanout() const { return _fanout; }
    int getWorkers() const { return _workers; }
    int getTimeout() const { return _timeout; }
    bool isTraceEnabled() const { return _trace; }
    bool hasDnsArgs() const { return _has_dns_args; }
    CacheCommand getCacheCommand() const { return _cache_command; }

    /**
     * @brief Imprime um resumo dos argumentos DNS configurados.
     */
    void printSummary() const;

    /**
     * @brief Imprime a mensagem de uso padrão da aplicação.
     */
    static void printUsage();
    
    /**
     * @brief Converte um enum Mode para sua representação em string.
     * @param m O modo a ser convertido.
     * @return A string correspondente.
     */
    static std::string modeToString(Mode m);

    /**
     * @brief Converte uma string para o enum Mode correspondente.
     * @param s A string a ser convertida.
     * @return O enum Mode.
     */
    static Mode stringToMode(const std::string& s);
    
private:
    void parse(int argc, char* argv[]);

    std::string _ns = "8.8.8.8"; // Default
    std::string _name = "example.com.";
    std::string _qtype = "A";
    Mode _mode = Mode::Recursive;
    std::optional<std::string> _sni;
    std::optional<std::string> _trust_anchor;
    int _fanout = 1;
    int _workers = 1;
    int _timeout = 5;
    bool _trace = false;

    bool _has_dns_args = false;
    CacheCommand _cache_command = CacheCommand::None;
};