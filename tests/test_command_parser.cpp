#include "gtest/gtest.h"
#include "../src/server/cache_command_parser.hpp" 

/**
 * @brief Testa o parsing de um comando simples sem argumentos.
 */
TEST(CommandParserTest, ParsesSimpleStatusCommand)
{
    std::string command_str = "STATUS";
    Command cmd = CommandParser::parse(command_str);

    ASSERT_EQ(cmd.type, CommandType::STATUS);
}

/**
 * @brief Testa o parsing de um comando com argumentos, incluindo um argumento com aspas.
 */
TEST(CommandParserTest, ParsesPutCommandWithQuotedArgument)
{
    std::string command_str = "PUT www.test.com 15 1 \"Preference: 10, Exchange: mail.test.com\" 300 0";
    Command cmd = CommandParser::parse(command_str);

    ASSERT_EQ(cmd.type, CommandType::PUT_POSITIVE);
    
    ASSERT_TRUE(cmd.key.has_value());
    EXPECT_EQ(cmd.key->qname, "www.test.com");
    EXPECT_EQ(cmd.key->qtype, 15);
    EXPECT_EQ(cmd.key->qclass, 1);

    ASSERT_TRUE(cmd.positive_entry.has_value());
    EXPECT_EQ(std::get<std::string>(cmd.positive_entry->rdata), "Preference: 10, Exchange: mail.test.com");
    EXPECT_EQ(cmd.positive_entry->is_dnssec_validated, false);
}

/**
 * @brief Testa se o parser lança uma exceção para um comando desconhecido.
 */
TEST(CommandParserTest, ThrowsOnInvalidCommand)
{
    std::string command_str = "COMANDO_QUE_NAO_EXISTE";
    ASSERT_THROW(CommandParser::parse(command_str), std::runtime_error);
}

/**
 * @brief Testa se o parser lança uma exceção para argumentos mal formatados.
 */
TEST(CommandParserTest, ThrowsOnMalformedArguments)
{
    std::string command_str = "SET POSITIVE ABC"; // ABC não é um número
    ASSERT_THROW(CommandParser::parse(command_str), std::runtime_error);
}