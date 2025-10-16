#include "stringUtils.hpp"

void convertCase(string& str, bool target_case) {
    auto converter = (target_case) ? ::tolower : ::toupper;

    transform(str.begin(), str.end(), str.begin(), 
        [converter](unsigned char c) {
            return converter(c);
        }
    );
}

uint16_t qtypeToUint16(const string& qtype_str)
{
    static const unordered_map<string, uint16_t> qtype_map = { {"A", 1}, {"NS", 2}, {"CNAME", 5}, {"SOA", 6}, {"MX", 15}, {"TXT", 16}, {"AAAA", 28} };

    auto it = qtype_map.find(qtype_str);

    if (it != qtype_map.end()) 
        return it->second;
    
    cerr << "Aviso: nao foi possivel reconhecer QTYPE '" << qtype_str << "'. Usando 'A' como padrão.\n" << endl;
    
    return 1;
}