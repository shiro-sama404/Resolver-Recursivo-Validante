#pragma once
#include <string>
#include <sstream>
#include <iostream>

#include "cache_store.hpp"
#include "../utils/stringUtils.hpp"

class CacheController
{
public:
    explicit CacheController(CacheStore& store);
    string processCommand(const string& cmd);

private:
    CacheStore& _store;

    string handleGet(const string& args, bool positive);
    string handlePut(const string& args, bool positive);
    string handlePurge(const string& args);
    string handleSet  (const string& args);
    string handleList (const string& args);
};