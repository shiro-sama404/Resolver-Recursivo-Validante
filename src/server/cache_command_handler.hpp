#pragma once
#include <iostream>

#include "arguments.hpp"
#include "cache_client.hpp"
#include "cache_server.hpp"
#include "../utils/consoleUtils.hpp"

class CacheCommandHandler
{
public:
    static int execute(const Arguments& args, int argc, char* argv[]);
};