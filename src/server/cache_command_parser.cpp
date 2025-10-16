#include "cache_command_parser.hpp"

#include <sstream>

using namespace std;

Command CommandParser::parse(const string& input_str)
{
    stringstream ss(input_str);
    string command_str;
    ss >> command_str;

    if (command_str == "PUT")
    {
        CacheKey key;
        PositiveCacheEntry entry;
        string rdata_str;
        int ttl;
        
        ss >> key.qname >> key.qtype >> key.qclass;

        ss >> ws;
        if (ss.peek() == '"') {
            ss.get(); 
            getline(ss, rdata_str, '"');
        }
        else
            ss >> rdata_str;

        ss >> ttl >> entry.is_dnssec_validated;
        if (ss.fail())
            throw runtime_error("Argumentos inválidos para PUT.");
        
        entry.rdata = rdata_str;
        entry.expiration_time = time(nullptr) + ttl;
        return { CommandType::PUT_POSITIVE, key, entry };
    }
    if (command_str == "PUT_NEGATIVE")
    {
        CacheKey key;
        NegativeCacheEntry entry;
        string reason_str;
        int ttl;

        ss >> key.qname >> key.qtype >> key.qclass >> reason_str >> ttl;
        if (ss.fail())
            throw runtime_error("Argumentos inválidos para PUT_NEGATIVE.");

        entry.reason = (reason_str == "NXDOMAIN") ? NegativeReason::NXDOMAIN : NegativeReason::NODATA;
        entry.expiration_time = time(nullptr) + ttl;
        return { CommandType::PUT_NEGATIVE, key, nullopt, entry };
    }
    if (command_str == "GET")
    {
        CacheKey key;
        ss >> key.qname >> key.qtype >> key.qclass;
        if (ss.fail())
            throw runtime_error("Argumentos inválidos para GET.");
        return { CommandType::GET_POSITIVE, key };
    }
    if (command_str == "GET_NEGATIVE")
    {
        CacheKey key;
        ss >> key.qname >> key.qtype >> key.qclass;
        if (ss.fail()) throw runtime_error("Argumentos inválidos para GET_NEGATIVE.");
        return { CommandType::GET_NEGATIVE, key };
    }
    if (command_str == "PURGE")
    {
        string target_str;
        ss >> target_str;
        return { CommandType::PURGE, nullopt, nullopt, nullopt, CacheStore::stringToTarget(target_str) };
    }
    if (command_str == "LIST")
    {
        string target_str;
        ss >> target_str;
        return { CommandType::LIST, nullopt, nullopt, nullopt, CacheStore::stringToTarget(target_str) };
    }
    if (command_str == "SET")
    {
        string target_str;
        size_t size;
        ss >> target_str >> size;
        if (ss.fail())
            throw runtime_error("Argumentos inválidos para SET.");
        bool is_pos = (target_str == "POSITIVE");
        return { CommandType::SET_MAX_SIZE, nullopt, nullopt, nullopt, nullopt, size, is_pos };
    }
    if (command_str == "STATUS")                return { CommandType::STATUS };
    if (command_str == "SHUTDOWN")              return { CommandType::SHUTDOWN };
    if (command_str == "START_CLEANUP_THREAD")  return { CommandType::START_CLEANUP_THREAD };
    
    throw runtime_error("Comando desconhecido: " + command_str);
}