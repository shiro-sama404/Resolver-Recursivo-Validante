#include "consoleUtils.hpp"

void exit_error_message(const string& msg, const bool exit_code = true) {
    cerr << msg << endl;
    if (exit_code)
        exit(EXIT_FAILURE);
}