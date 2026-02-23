// ============================================================================
// Úloha 1: Maticové násobenie — SEKVENČNÁ verzia (C++)
// ============================================================================
// Kompilácia: g++ -O2 -o main_seq src/main_seq.cpp
// Spustenie:  ./main_seq [veľkosť] [int|float]
// ============================================================================

#include "matrix.h"
#include <string>

template <typename T>
void run_benchmark(int size, const std::string& type_name) {
    std::cout << "\n=== Sekvenčné násobenie " << size << "x" << size
              << " (" << type_name << ") ===" << std::endl;

    // Generovanie matíc
    auto A = generate_matrix<T>(size, size);
    auto B = generate_matrix<T>(size, size);

    // Meranie
    std::vector<std::vector<T>> C;
    double time_ms = measure_time([&]() {
        C = multiply_seq(A, B);
    });

    print_result("sequential", size, type_name, 1, time_ms);
}

int main(int argc, char* argv[]) {
    std::vector<int> sizes = {500, 1000, 2000};

    // Ak je zadaná veľkosť ako argument, použi len tú
    if (argc >= 2) {
        sizes = {std::stoi(argv[1])};
    }

    std::string dtype = "all";
    if (argc >= 3) {
        dtype = argv[2];
    }

    for (int size : sizes) {
        if (dtype == "all" || dtype == "int") {
            run_benchmark<int>(size, "int");
        }
        if (dtype == "all" || dtype == "float") {
            run_benchmark<float>(size, "float");
        }
    }

    return 0;
}
