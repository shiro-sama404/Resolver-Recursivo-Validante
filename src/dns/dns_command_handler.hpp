#pragma once
#include <iostream>

#include "arguments.hpp"
#include "dns_client.hpp"
#include "dns_mensagem.hpp"
#include "dot_cliente.hpp"
#include "../utils/consoleUtils.hpp"
#include "../utils/stringUtils.hpp"

using namespace std;

class DNSCommandHandler
{
public:
    static int execute(const Arguments& args, int argc, char* argv[]);
};