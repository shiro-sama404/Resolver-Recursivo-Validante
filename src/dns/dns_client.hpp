#pragma once

#include <optional>
#include <string>
#include <vector>

#include "dns_message.hpp"
#include "dot_client.hpp"

#define MAX_DEPTH 30

/**
 * @brief Um motor de resolução DNS que lida com a comunicação de rede UDP/TCP/DoT.
 * @details Esta classe é responsável por executar o processo de resolução, seguindo
 * delegações e CNAMEs. Não possui conhecimento sobre o sistema de cache.
 */
class DNSClient
{
public:
    DNSClient() = default;

    /**
     * @brief Executa o processo de resolução de um nome de domínio.
     * @param domain_name O nome de domínio a ser resolvido.
     * @param qtype O tipo de registro solicitado (ex: 1 para A, 28 para AAAA).
     * @return Um vetor de bytes contendo a resposta DNS bruta. Retorna vazio em caso de falha.
     */
    std::vector<uint8_t> resolve(const std::string& domain_name, uint16_t qtype);

    // --- Métodos de Configuração ---
    void setRecursion(bool is_recursive) { _is_recursive = is_recursive; }
    void setUseDot(bool use_dot) { _use_dot = use_dot; }
    void setValidateDnssec(bool validate) { _validate_dnssec = validate; }
    void setStrictDnssec(bool is_strict) { _is_strict_dnssec = is_strict; }
    void setForwarder(const std::string& ip) { _forwarder_ip = ip; }
    void setSni(const std::string& sni) { _sni = sni; }

private:
    bool _is_recursive = true;
    bool _use_dot = false;
    bool _validate_dnssec = false;
    bool _is_strict_dnssec = false;
    std::string _forwarder_ip;
    std::string _sni;

    /**
     * @brief Define os possíveis estados durante o processo de resolução iterativa.
     */
    enum class ResolutionState
    {
        AnswerFound,
        Delegation,
        CnameRedirect,
        Error
    };

    // --- Métodos de Rede ---
    std::vector<uint8_t> queryUdp(const std::vector<uint8_t>& query_packet, const std::string& server_ip, int timeout_sec = 5);
    std::vector<uint8_t> queryTcp(const std::vector<uint8_t>& query_packet, const std::string& server_ip, int timeout_sec = 5);
    std::vector<uint8_t> queryDot(const std::vector<uint8_t>& query_packet, const std::string& server_ip, int timeout_sec = 5);

    // --- Métodos de Lógica de Resolução ---
    ResolutionState processResponse(const DNSMessage& response,
        std::vector<std::string>& current_nameservers,
        std::string& next_query_name);

    std::vector<std::string> getDelegatedNsIps(const DNSMessage& response_message);
};