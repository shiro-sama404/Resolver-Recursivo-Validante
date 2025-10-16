#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../utils/cache_types.hpp"

/**
 * @brief Gerencia o armazenamento e ciclo de vida de registros DNS em cache.
 * @details Oferece caches separadas para respostas positivas e negativas,
 * com controle de tamanho, expiração e limpeza automática (thread-safe).
 */
class CacheStore
{
public:
    CacheStore() {};

    /**
     * @brief Insere ou atualiza uma entrada na cache positiva.
     * @param key A chave única da consulta.
     * @param entry O registro de resposta positiva a ser armazenado.
     */
    void put(const CacheKey& key, const PositiveCacheEntry& entry);

    /**
     * @brief Insere ou atualiza uma entrada na cache negativa.
     * @param key A chave única da consulta.
     * @param entry O registro de resposta negativa a ser armazenado.
     */
    void put(const CacheKey& key, const NegativeCacheEntry& entry);
    
    /**
     * @brief Busca uma entrada na cache positiva.
     * @param key A chave da consulta a ser buscada.
     * @return Um std::optional contendo a entrada se encontrada e não expirada.
     */
    std::optional<PositiveCacheEntry> getPositive(const CacheKey& key);

    /**
     * @brief Busca uma entrada na cache negativa.
     * @param key A chave da consulta a ser buscada.
     * @return Um std::optional contendo a entrada se encontrada e não expirada.
     */
    std::optional<NegativeCacheEntry> getNegative(const CacheKey& key);
    
    /**
     * @brief Limpa todos os registros de uma ou de ambas as caches.
     * @param target A cache a ser limpa (Positive, Negative, ou All).
     */
    void purge(CacheTarget target);

    /**
     * @brief Lista o conteúdo bruto de uma ou ambas as caches.
     * @param target A cache a ser listada.
     * @return Um par de vetores contendo as entradas das caches.
     */
    std::pair<std::vector<std::pair<CacheKey, PositiveCacheEntry>>, std::vector<std::pair<CacheKey, NegativeCacheEntry>>> list(CacheTarget target);
    
    /**
     * @brief Define o número máximo de entradas para uma das caches.
     * @param n O novo tamanho máximo.
     * @param is_positive Verdadeiro para a cache positiva, falso para a negativa.
     */
    void setMaxSize(size_t n, bool is_positive);

    /**
     * @brief Obtém o status atual das caches (tamanho e capacidade).
     * @return Uma struct CacheStatus com as informações.
     */
    CacheStatus getStatus() const;

    /**
     * @brief Inicia uma thread em segundo plano para limpar entradas expiradas.
     * @param interval_sec O intervalo em segundos entre cada limpeza.
     */
    void startCleanupThread(int interval_sec = 10);

    /**
     * @brief Para a thread de limpeza, se estiver em execução.
     */
    void stopCleanupThread();

    /**
     * @brief Converte uma string para o enum CacheTarget.
     * @param s A string a ser convertida (ex: "POSITIVE").
     * @return O valor do enum correspondente.
     */
    static CacheTarget stringToTarget(const std::string& s);

private:
    /**
     * @brief Método executado pela thread para remover entradas expiradas.
     */
    void cleanup();

    std::unordered_map<CacheKey, PositiveCacheEntry> _positive_cache;
    std::unordered_map<CacheKey, NegativeCacheEntry> _negative_cache;
    mutable std::mutex _mutex;

    size_t _max_positive_size = 50;
    size_t _max_negative_size = 50;
    
    bool _is_running = false;
    std::thread _cleanup_thread;
};