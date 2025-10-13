#pragma once
#include <iostream>

#include "arguments.hpp"
#include "dns_mensagem.hpp"
#include "client.hpp"
#include "dot_cliente.hpp"
#include "../utils/consoleUtils.hpp"
#include "../utils/stringUtils.hpp"

class DNSCommandHandler
{
public:
    static int execute(const Arguments& args, int argc, char* argv[]);
};