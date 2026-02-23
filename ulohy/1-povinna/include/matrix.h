#pragma once

#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <functional>

// ============================================================================
// Pomocné funkcie pre maticové násobenie
// ============================================================================

/// Generovanie matice s náhodnými hodnotami
template <typename T>
std::vector<std::vector<T>> generate_matrix(int rows, int cols, T min_val = 0, T max_val = 100) {
    std::mt19937 rng(42); // fixný seed pre reprodukovateľnosť
    std::uniform_int_distribution<int> dist(static_cast<int>(min_val), static_cast<int>(max_val));

    std::vector<std::vector<T>> mat(rows, std::vector<T>(cols));
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            mat[i][j] = static_cast<T>(dist(rng));
    return mat;
}

/// Sekvenčné maticové násobenie C = A * B
template <typename T>
std::vector<std::vector<T>> multiply_seq(const std::vector<std::vector<T>>& A,
                                          const std::vector<std::vector<T>>& B) {
    int n = A.size();
    int m = B[0].size();
    int k = B.size();
    std::vector<std::vector<T>> C(n, std::vector<T>(m, 0));

    for (int i = 0; i < n; ++i)
        for (int p = 0; p < k; ++p)       // ikj order pre lepšiu cache locality
            for (int j = 0; j < m; ++j)
                C[i][j] += A[i][p] * B[p][j];

    return C;
}

/// Meranie času funkcie (vracia čas v milisekundách)
template <typename Func>
double measure_time(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

/// Porovnanie dvoch matíc (pre overenie správnosti)
template <typename T>
bool matrices_equal(const std::vector<std::vector<T>>& A,
                    const std::vector<std::vector<T>>& B, T epsilon = 1) {
    if (A.size() != B.size()) return false;
    for (size_t i = 0; i < A.size(); ++i) {
        if (A[i].size() != B[i].size()) return false;
        for (size_t j = 0; j < A[i].size(); ++j) {
            T diff = A[i][j] > B[i][j] ? A[i][j] - B[i][j] : B[i][j] - A[i][j];
            if (diff > epsilon) return false;
        }
    }
    return true;
}

/// Výpis výsledkov merania
inline void print_result(const std::string& label, int size, const std::string& dtype,
                         int threads, double time_ms) {
    std::cout << std::left << std::setw(20) << label
              << " | size=" << std::setw(5) << size
              << " | type=" << std::setw(6) << dtype
              << " | threads=" << std::setw(2) << threads
              << " | time=" << std::fixed << std::setprecision(2) << time_ms << " ms"
              << std::endl;
}
