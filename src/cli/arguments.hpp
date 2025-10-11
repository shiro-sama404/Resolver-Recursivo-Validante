#pragma once

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <getopt.h>

using namespace std;

enum class Mode
{
    Recursive,
    Forwarder,
    Iterative,
    Validating,
    Insecure,
    StrictDNSSEC,
    Dot,
    Unknown
};

enum class CacheCommand
{
    None,
    Activate,
    Deactivate,
    Status,
    SetPositive,
    SetNegative,
    PurgePositive,
    PurgeNegative,
    PurgeAll,
    ListPositive,
    ListNegative,
    ListAll
};

class Arguments
{
public:
    Arguments(int argc, char* argv[]);

    // Getters
    const string& get_ns() const { return _ns; }
    const string& get_name() const { return _name; }
    const string& get_qtype() const { return _qtype; }
    Mode get_mode() const { return _mode; }
    const optional<string>& get_sni() const { return _sni; }
    const optional<string>& get_trust_anchor() const { return _trust_anchor; }
    int get_fanout() const { return _fanout; }
    int get_workers() const { return _workers; }
    int get_timeout() const { return _timeout; }
    bool is_trace_enabled() const { return _trace; }

    CacheCommand get_cache_command() const { return cacheCommand_; }
    int get_cache_value() const { return cacheValue_; }

    void print_summary() const;
    static void print_usage(const char* prog);
    
    static string mode_to_string(Mode m);
    static Mode string_to_mode(const string& s);

private:
    string _ns;
    string _name;
    string _qtype;
    Mode _mode = Mode::Unknown;
    optional<string> _sni;
    optional<string> _trust_anchor;
    int _fanout = 1;
    int _workers = 1;
    int _timeout = 5;
    bool _trace = false;

    CacheCommand cacheCommand_ = CacheCommand::None;
    int cacheValue_ = 0; // set positive/negative

    void parse(int argc, char* argv[]);
};
