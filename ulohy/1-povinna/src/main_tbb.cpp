// ============================================================================
// Úloha 1: Maticové násobenie — PARALELNÁ verzia (Intel TBB)
// ============================================================================
// Kompilácia: g++ -O2 -o main_tbb src/main_tbb.cpp -ltbb
// Spustenie:  ./main_tbb [veľkosť] [int|float] [vlákna]
// ============================================================================

#include "matrix.h"
#include <tbb/tbb.h>
#include <tbb/global_control.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <string>

/// Paralelné maticové násobenie pomocou TBB (2D blocked range)
template <typename T>
std::vector<std::vector<T>> multiply_tbb(const std::vector<std::vector<T>>& A,
                                          const std::vector<std::vector<T>>& B) {
    int n = A.size();
    int m = B[0].size();
    int k = B.size();
    std::vector<std::vector<T>> C(n, std::vector<T>(m, 0));

    tbb::parallel_for(
        tbb::blocked_range2d<int>(0, n, 0, m),
        [&](const tbb::blocked_range2d<int>& r) {
            for (int i = r.rows().begin(); i < r.rows().end(); ++i) {
                for (int p = 0; p < k; ++p) {
                    for (int j = r.cols().begin(); j < r.cols().end(); ++j) {
                        C[i][j] += A[i][p] * B[p][j];
                    }
                }
            }
        }
    );

    return C;
}

template <typename T>
void run_benchmark(int size, const std::string& type_name, int num_threads) {
    // Obmedzenie počtu vlákien
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism, num_threads);

    std::cout << "\n=== TBB násobenie " << size << "x" << size
              << " (" << type_name << ", " << num_threads << " vlákien) ===" << std::endl;

    // Generovanie matíc
    auto A = generate_matrix<T>(size, size);
    auto B = generate_matrix<T>(size, size);

    // Meranie paralelnej verzie
    std::vector<std::vector<T>> C_tbb;
    double time_tbb = measure_time([&]() {
        C_tbb = multiply_tbb(A, B);
    });
    print_result("tbb", size, type_name, num_threads, time_tbb);

    // Meranie sekvenčnej verzie pre porovnanie
    std::vector<std::vector<T>> C_seq;
    double time_seq = measure_time([&]() {
        C_seq = multiply_seq(A, B);
    });
    print_result("sequential", size, type_name, 1, time_seq);

    // Overenie správnosti
    bool correct = matrices_equal(C_seq, C_tbb);
    std::cout << "Správnosť: " << (correct ? "OK" : "CHYBA!") << std::endl;

    // Speedup
    double speedup = time_seq / time_tbb;
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
}

int main(int argc, char* argv[]) {
    std::vector<int> sizes = {500, 1000, 2000};
    std::vector<int> threads = {2, 4, 8};

    if (argc >= 2) sizes = {std::stoi(argv[1])};

    std::string dtype = "all";
    if (argc >= 3) dtype = argv[2];

    if (argc >= 4) threads = {std::stoi(argv[3])};

    for (int size : sizes) {
        for (int t : threads) {
            if (dtype == "all" || dtype == "int") {
                run_benchmark<int>(size, "int", t);
            }
            if (dtype == "all" || dtype == "float") {
                run_benchmark<float>(size, "float", t);
            }
        }
    }

    return 0;
}
