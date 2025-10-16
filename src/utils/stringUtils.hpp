#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

void convertCase(string& str, bool target_case);

uint16_t qtypeToUint16(const string& qtype_str); //Para traduzir a string