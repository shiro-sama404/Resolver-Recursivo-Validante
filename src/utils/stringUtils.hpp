#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

void convert_case(string& str, bool target_case);

uint16_t qtype_to_uint16(const string& qtype_str); //Para traduzir a string