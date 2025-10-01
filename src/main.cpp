#include "cli/arguments.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        Arguments args(argc, argv);
        args.print_summary();

        if (args.get_mode() == Mode::Validating) {
            cout << "DNSSEC ativado\n";
        }
    }
    catch (const std::exception& ex) {
        cerr << "Erro: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}