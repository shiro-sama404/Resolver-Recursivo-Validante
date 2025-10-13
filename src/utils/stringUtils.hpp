#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <cstdint>

using namespace std;

void convert_case(string& str, bool target_case);

uint16_t qtype_to_uint16(const string& qtype_str); //Para traduzir a string