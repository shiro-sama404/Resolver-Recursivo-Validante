#include "consoleUtils.hpp"

void exitErrorMessage(const string& msg, const bool exit_code) {
    cerr << msg << endl;
    if (exit_code)
        exit(EXIT_FAILURE);
}