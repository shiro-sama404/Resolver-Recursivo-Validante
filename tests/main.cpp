#include "gtest/gtest.h"

/**
 * @brief Ponto de entrada principal para o executável de testes.
 * @details Inicializa o Google Test e executa todos os testes descobertos.
 * @param argc Número de argumentos.
 * @param argv Vetor de argumentos.
 * @return O resultado da execução dos testes.
 */
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}