#include "gtest/gtest.h"
#include "../src/dns/dns_message.hpp"
/**
 * @brief Testa se o método buildQuery codifica corretamente o nome de domínio.
 */
TEST(DNSMessageTest, BuildQueryEncodesDomainNameCorrectly)
{
    DNSMessage msg;
    msg.configureQuery("www.google.com", static_cast<uint16_t>(DNSRecordType::A));

    std::vector<uint8_t> query_packet = msg.buildQuery();

    ASSERT_EQ(query_packet.size(), 32);

    // Verifica a codificação do nome (começa no byte 12)
    std::vector<uint8_t> expected_name = {
        3, 'w', 'w', 'w', 6, 'g', 'o', 'o', 'g', 'l', 'e', 3, 'c', 'o', 'm', 0
    };
    for (size_t i = 0; i < expected_name.size(); ++i)
    {
        ASSERT_EQ(query_packet[12 + i], expected_name[i]);
    }

    // Verifica QTYPE e QCLASS no final do pacote
    ASSERT_EQ(query_packet[28], 0x00); // QTYPE (A) High Byte
    ASSERT_EQ(query_packet[29], 0x01); // QTYPE (A) Low Byte
    ASSERT_EQ(query_packet[30], 0x00); // QCLASS (IN) High Byte
    ASSERT_EQ(query_packet[31], 0x01); // QCLASS (IN) Low Byte
}

/**
 * @brief Testa se o método parseResponse decodifica corretamente uma resposta simples.
 */
TEST(DNSMessageTest, ParseSimpleARecordResponse)
{
    std::vector<uint8_t> raw_response = {
        // Cabeçalho (12 bytes)
        0x00, 0x01, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        // Pergunta (7 bytes)
        0x01, 'a', 0x03, 'c', 'o', 'm', 0x00, 0x00, 0x01, 0x00, 0x01,
        // Resposta (16 bytes)
        0xc0, 0x0c, // Ponteiro para "a.com"
        0x00, 0x01, // Tipo A
        0x00, 0x01, // Classe IN
        0x00, 0x00, 0x01, 0x2c, // TTL: 300
        0x00, 0x04, // Tamanho do dado: 4
        0x01, 0x02, 0x03, 0x04  // IP: 1.2.3.4
    };

    DNSMessage msg;
    msg.parseResponse(raw_response);

    ASSERT_EQ(msg.header.answer_count, 1);
    ASSERT_EQ(msg.answers.size(), 1);
    
    const auto& answer = msg.answers[0];
    ASSERT_EQ(answer.name, "a.com"); // Testa a descompressão do nome
    ASSERT_EQ(answer.type, static_cast<uint16_t>(DNSRecordType::A));
    ASSERT_EQ(answer.ttl, 300);
    ASSERT_EQ(answer.parsed_data, "1.2.3.4");
}