#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Enum fortemente tipado para os tipos de registros DNS mais comuns.
 * @details Melhora a legibilidade e a segurança de tipo em relação ao uso de "magic numbers".
 */
enum class DNSRecordType : uint16_t
{
    A = 1,
    NS = 2,
    CNAME = 5,
    SOA = 6,
    MX = 15,
    TXT = 16,
    AAAA = 28,
    OPT = 41,
    DS = 43,
    RRSIG = 46,
    DNSKEY = 48,
    Unsupported = 0
};

/**
 * @brief Representa o cabeçalho de uma mensagem DNS (12 bytes).
 */
struct DNSHeader
{
    uint16_t id;
    uint16_t flags;
    uint16_t query_domain_count;
    uint16_t answer_count;
    uint16_t name_server_count;
    uint16_t additional_count;
};

/**
 * @brief Representa a seção de pergunta de uma mensagem DNS.
 */
struct DNSQuestion
{
    std::string name;
    uint16_t type;
    uint16_t class_code;
};

/**
 * @brief Representa um Resource Record (RR) genérico das seções de resposta,
 * autoridade ou adicionais.
 */
struct ResourceRecord
{
    std::string name;
    uint16_t type;
    uint16_t class_code;
    uint32_t ttl;
    uint16_t data_length;
    std::vector<uint8_t> raw_data;
    std::string parsed_data; // Armazena a resposta decodificada e legível
};

/**
 * @brief Representa uma opção dentro de um registro EDNS (OPT).
 */
struct EDNSOption
{
    uint16_t code;
    std::vector<uint8_t> data;
};

/**
 * @brief Modela uma mensagem DNS completa, com métodos para construir queries e
 * analisar respostas.
 */
class DNSMessage
{
public:
    DNSHeader header;
    DNSQuestion question;
    std::vector<ResourceRecord> answers;
    std::vector<ResourceRecord> authorities;
    std::vector<ResourceRecord> additionals;
    std::vector<EDNSOption> edns_options;

    /**
     * @brief Construtor padrão. Inicializa uma query DNS básica.
     */
    DNSMessage();

    /**
     * @brief Configura a mensagem para uma nova consulta.
     * @param name O nome de domínio a ser consultado.
     * @param type O tipo de registro (ex: DNSRecordType::A).
     */
    void configureQuery(const std::string& name, uint16_t type);

    /**
     * @brief Constrói o pacote de bytes da consulta DNS para envio pela rede.
     * @return Um vetor de bytes representando a query.
     */
    std::vector<uint8_t> buildQuery();

    /**
     * @brief Analisa um pacote de bytes de resposta DNS e preenche a estrutura da classe.
     * @param data O vetor de bytes recebido da rede.
     */
    void parseResponse(const std::vector<uint8_t>& data);

    /**
     * @brief Imprime um resumo formatado e legível da mensagem DNS no console.
     */
    void printResponse() const;

    /**
     * @brief Extrai o RCODE (código de resposta) das flags do cabeçalho.
     * @return O valor do RCODE (0 para sucesso, 3 para NXDOMAIN, etc.).
     */
    uint8_t getRcode() const;

private:
    uint16_t _edns_udp_size = 512;
    uint8_t  _edns_version = 0;
    uint16_t _edns_z_field = 0;

    // --- Métodos Auxiliares de Construção (Build) ---
    void addUint16(std::vector<uint8_t>& packet, uint16_t value);
    void addQuestion(std::vector<uint8_t>& packet);

    // --- Métodos Auxiliares de Análise (Parse) ---
    static uint16_t readUint16(const std::vector<uint8_t>& data, size_t& pos);
    static uint32_t readUint32(const std::vector<uint8_t>& data, size_t& pos);
    static std::string readName(const std::vector<uint8_t>& data, size_t& pos);

    void readHeader(const std::vector<uint8_t>& data, size_t& pos);
    void readQuestion(const std::vector<uint8_t>& data, size_t& pos);
    void readAnswers(const std::vector<uint8_t>& data, size_t& pos);
    void readAuthorities(const std::vector<uint8_t>& data, size_t& pos);
    void readAdditionals(const std::vector<uint8_t>& data, size_t& pos);

    ResourceRecord readRecord(const std::vector<uint8_t>& data, size_t& pos);

    // --- Métodos de Decodificação de RDATA ---
    void decodeA     (ResourceRecord& rr);
    void decodeAAAA  (ResourceRecord& rr);
    void decodeCNAME (ResourceRecord& rr);
    void decodeDS    (ResourceRecord& rr);
    void decodeNS    (ResourceRecord& rr);
    void decodeDNSKEY(ResourceRecord& rr);
    void decodeMX    (ResourceRecord& rr);
    void decodeRRSIG (ResourceRecord& rr);
    void decodeSOA   (ResourceRecord& rr);
    void decodeTXT   (ResourceRecord& rr);
    void decodeOPT   (ResourceRecord& rr);
};