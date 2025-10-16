#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// =========================================================================
//                  ESTRUTURAS FUNDAMENTAIS DA CACHE
// =========================================================================

/**
 * @brief Representa a chave única para uma entrada de cache.
 * @details Combina os 3 campos da seção de pergunta de uma consulta DNS
 * para identificar um registro de forma única.
 */
struct CacheKey
{
    std::string qname;
    uint16_t qtype;
    uint16_t qclass;

    /**
     * @brief Operador de igualdade para comparações em mapas hash.
     * @return Verdadeiro se todos os campos forem iguais.
     */
    bool operator==(const CacheKey& other) const
    {
        return qname == other.qname && qtype == other.qtype && qclass == other.qclass;
    }
};

namespace std
{
    /**
     * @brief Especialização da função de hash para que CacheKey possa ser usada
     * em um std::unordered_map.
     */
    template <>
    struct hash<CacheKey>
    {
        size_t operator()(const CacheKey& k) const
        {
            size_t h1 = std::hash<std::string>()(k.qname);
            size_t h2 = std::hash<uint16_t>()(k.qtype);
            size_t h3 = std::hash<uint16_t>()(k.qclass);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

/** @brief Alias para os dados de resposta, usando um variant seguro. */
using RecordDataVariant = std::variant<std::string, std::vector<uint8_t>>;

/**
 * @brief Representa uma entrada na cache positiva.
 */
struct PositiveCacheEntry
{
    RecordDataVariant rdata;
    time_t expiration_time;
    bool is_dnssec_validated;
};

/**
 * @brief Enum para os tipos de resposta negativa.
 */
enum NegativeReason { NXDOMAIN, NODATA };

/**
 * @brief Representa uma entrada na cache negativa.
 */
struct NegativeCacheEntry
{
    NegativeReason reason;
    time_t expiration_time;
};

/**
 * @brief Contém as estatísticas de uso e capacidade da cache.
 */
struct CacheStatus
{
    size_t positive_current_size;
    size_t positive_max_size;
    size_t negative_current_size;
    size_t negative_max_size;
};

/**
 * @brief Define o alvo de uma operação (positiva, negativa ou ambas).
 */
enum class CacheTarget { Positive, Negative, All, Invalid };

// =========================================================================
//                  ESTRUTURAS DE CONTROLE DA CACHE
// =========================================================================

/**
 * @brief Enum dos tipos de comandos que o controller pode processar.
 */
enum class CommandType
{
    PUT_POSITIVE,
    PUT_NEGATIVE,
    GET_POSITIVE,
    GET_NEGATIVE,
    PURGE,
    SET_MAX_SIZE,
    LIST,
    STATUS,
    SHUTDOWN,
    START_CLEANUP_THREAD
};

/**
 * @brief Estrutura que encapsula um comando e todos os seus possíveis argumentos.
 */
struct Command
{
    CommandType type;
    std::optional<CacheKey> key;
    std::optional<PositiveCacheEntry> positive_entry;
    std::optional<NegativeCacheEntry> negative_entry;
    std::optional<CacheTarget> target;
    std::optional<size_t> size;
    std::optional<bool> is_positive;
    std::optional<int> interval;
};

/**
 * @brief Estrutura que encapsula a resposta de um comando.
 */
struct CacheResponse
{
    std::string message;
    std::optional<PositiveCacheEntry> positive_entry;
    std::optional<NegativeCacheEntry> negative_entry;
};
