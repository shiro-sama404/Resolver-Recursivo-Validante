#pragma once

#include <stdexcept>
#include <string>

#include "cache_controller.hpp"

/**
 * @brief Classe estática para analisar comandos de texto e convertê-los em objetos Command.
 */
class CommandParser
{
public:
    /**
     * @brief Analisa uma string de entrada e a converte em um objeto Command.
     * @param input_str A string de comando bruta recebida do cliente.
     * @return O objeto Command preenchido.
     * @throw std::runtime_error se o comando for inválido ou mal formatado.
     */
    static Command parse(const std::string& input_str);
};