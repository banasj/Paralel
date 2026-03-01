#pragma once

#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <atomic>

// ============================================================================
// Pomocné funkcie pre problém hodujúcich filozofov (Úloha 3)
// ============================================================================

/// Režim synchronizácie
enum SyncMode {
    SYNC_NONE,      // žiadne mutexy — úmyselne data race na demonštráciu
    SYNC_ORDERED    // mutexy + resource ordering — korektné riešenie
};

/// Štatistiky filozofa
struct PhilosopherStats {
    int id = 0;
    int meals_eaten = 0;
    double total_eating_ms = 0.0;
    double total_thinking_ms = 0.0;
    double total_waiting_ms = 0.0;
};

/// Výsledok paralelnej simulácie
struct DiningResult {
    std::vector<PhilosopherStats> stats;
    int total_meals_shared = 0;     // zdieľaný counter (int, nie atomic!) — data race bez mutexu
    int fork_violations = 0;        // koľkokrát 2 filozofi držali tú istú vidličku súčasne
    double time_ms = 0.0;
};

/// Konfigurácia simulácie
struct DiningConfig {
    int num_philosophers;
    int num_meals;          // koľkokrát má každý filozof jesť
    int think_intensity;    // intenzita výpočtu myslenia
    int eat_intensity;      // intenzita výpočtu jedenia
};

/// Simulácia výpočtovej záťaže (myslenie alebo jedenie)
inline void simulate_work(int intensity) {
    volatile double val = 1.0;
    for (int i = 0; i < intensity; ++i) {
        val = std::sin(val) * std::cos(val) + std::sqrt(std::abs(val) + 1.0);
    }
}

/// Výpis štatistík filozofov
inline void print_dining_stats(const std::vector<PhilosopherStats>& stats) {
    std::cout << "--- Štatistiky filozofov ---" << std::endl;
    std::cout << std::left
              << std::setw(6)  << "ID"
              << std::setw(10) << "Jedál"
              << std::setw(16) << "Jedenie [ms]"
              << std::setw(16) << "Myslenie [ms]"
              << std::setw(16) << "Čakanie [ms]"
              << std::endl;
    for (const auto& s : stats) {
        std::cout << std::left
                  << std::setw(6)  << s.id
                  << std::setw(10) << s.meals_eaten
                  << std::setw(16) << std::fixed << std::setprecision(1) << s.total_eating_ms
                  << std::setw(16) << s.total_thinking_ms
                  << std::setw(16) << s.total_waiting_ms
                  << std::endl;
    }
}

/// Kontrola férovosti — rozdiel medzi filozofmi
inline void check_fairness(const std::vector<PhilosopherStats>& stats) {
    int min_meals = stats[0].meals_eaten, max_meals = stats[0].meals_eaten;
    double min_wait = stats[0].total_waiting_ms, max_wait = stats[0].total_waiting_ms;
    for (const auto& s : stats) {
        min_meals = std::min(min_meals, s.meals_eaten);
        max_meals = std::max(max_meals, s.meals_eaten);
        min_wait = std::min(min_wait, s.total_waiting_ms);
        max_wait = std::max(max_wait, s.total_waiting_ms);
    }
    std::cout << "Férovosť jedál: min=" << min_meals << " max=" << max_meals
              << " (rozdiel: " << (max_meals - min_meals) << ")" << std::endl;
    std::cout << "Férovosť čakania: min=" << std::fixed << std::setprecision(1)
              << min_wait << " ms, max=" << max_wait << " ms" << std::endl;
}

/// Kontrola správnosti — každý filozof zjedol správny počet jedál
inline bool verify_results(const std::vector<PhilosopherStats>& stats, int expected_meals) {
    for (const auto& s : stats) {
        if (s.meals_eaten != expected_meals) return false;
    }
    return true;
}

/// Rozšírená kontrola správnosti pre paralelnú verziu
inline void verify_sync_results(const DiningResult& result, int expected_meals, int num_philosophers) {
    int expected_total = num_philosophers * expected_meals;

    // 1. Kontrola per-filozof počítadiel
    bool per_phil_ok = verify_results(result.stats, expected_meals);
    int actual_sum = 0;
    for (const auto& s : result.stats) actual_sum += s.meals_eaten;

    // 2. Kontrola zdieľaného počítadla (data race dôkaz)
    bool shared_ok = (result.total_meals_shared == expected_total);

    // 3. Kontrola fork violations
    bool forks_ok = (result.fork_violations == 0);

    std::cout << "--- Verifikácia ---" << std::endl;
    std::cout << "Per-filozof jedlá:   " << (per_phil_ok ? "OK" : "CHYBA!")
              << " (súčet: " << actual_sum << " / " << expected_total << ")" << std::endl;
    std::cout << "Zdieľaný counter:    " << (shared_ok ? "OK" : "CHYBA!")
              << " (" << result.total_meals_shared << " / " << expected_total;
    if (!shared_ok) {
        int lost = expected_total - result.total_meals_shared;
        std::cout << ", stratených: " << lost
                  << " = " << std::fixed << std::setprecision(1)
                  << (100.0 * lost / expected_total) << "%";
    }
    std::cout << ")" << std::endl;
    std::cout << "Fork violations:     " << (forks_ok ? "OK (0)" : "CHYBA!")
              << (!forks_ok ? " (" + std::to_string(result.fork_violations) + " violations)" : "")
              << std::endl;
    std::cout << "Celková správnosť:   "
              << ((per_phil_ok && shared_ok && forks_ok) ? "PASS ✓" : "FAIL ✗")
              << std::endl;
}

/// Výpis porovnávacej tabuľky dvoch režimov
inline void print_comparison(const DiningResult& sync_result, const DiningResult& nosync_result,
                             double seq_time, int expected_total) {
    std::cout << "\n┌──────────────────┬──────────────────┬──────────────────┐" << std::endl;
    std::cout << "│                  │  SYNC_ORDERED    │  SYNC_NONE       │" << std::endl;
    std::cout << "├──────────────────┼──────────────────┼──────────────────┤" << std::endl;
    std::cout << "│ Čas [ms]         │ " << std::setw(16) << std::fixed << std::setprecision(2)
              << sync_result.time_ms << " │ " << std::setw(16) << nosync_result.time_ms << " │" << std::endl;
    std::cout << "│ Speedup vs seq   │ " << std::setw(15) << std::setprecision(2)
              << (seq_time / sync_result.time_ms) << "x │ " << std::setw(15)
              << (seq_time / nosync_result.time_ms) << "x │" << std::endl;
    std::cout << "│ Správnosť        │ " << std::setw(16)
              << (sync_result.fork_violations == 0 && sync_result.total_meals_shared == expected_total ? "OK" : "CHYBA")
              << " │ " << std::setw(16)
              << (nosync_result.fork_violations == 0 && nosync_result.total_meals_shared == expected_total ? "OK" : "CHYBA")
              << " │" << std::endl;
    std::cout << "│ Fork violations  │ " << std::setw(16) << sync_result.fork_violations
              << " │ " << std::setw(16) << nosync_result.fork_violations << " │" << std::endl;
    std::cout << "│ Shared counter   │ " << std::setw(16) << sync_result.total_meals_shared
              << " │ " << std::setw(16) << nosync_result.total_meals_shared << " │" << std::endl;
    std::cout << "│ Očakávaný total  │ " << std::setw(16) << expected_total
              << " │ " << std::setw(16) << expected_total << " │" << std::endl;
    std::cout << "└──────────────────┴──────────────────┴──────────────────┘" << std::endl;
}

/// Sekvenčná simulácia — filozofi jedia jeden po druhom (žiadna kontencia)
inline void dining_sequential(const DiningConfig& cfg, std::vector<PhilosopherStats>& stats) {
    int n = cfg.num_philosophers;
    stats.resize(n);
    for (int i = 0; i < n; ++i) stats[i].id = i;

    for (int meal = 0; meal < cfg.num_meals; ++meal) {
        for (int i = 0; i < n; ++i) {
            // Myslenie
            auto t0 = std::chrono::high_resolution_clock::now();
            simulate_work(cfg.think_intensity);
            auto t1 = std::chrono::high_resolution_clock::now();
            stats[i].total_thinking_ms +=
                std::chrono::duration<double, std::milli>(t1 - t0).count();

            // Jedenie (žiadne čakanie na vidličky v sekvenčnej verzii)
            auto t2 = std::chrono::high_resolution_clock::now();
            simulate_work(cfg.eat_intensity);
            auto t3 = std::chrono::high_resolution_clock::now();
            stats[i].total_eating_ms +=
                std::chrono::duration<double, std::milli>(t3 - t2).count();
            stats[i].meals_eaten++;
        }
    }
}

/// Meranie času
template <typename Func>
double measure_time(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}
