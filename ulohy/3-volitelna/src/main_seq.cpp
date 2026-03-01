// ============================================================================
// Úloha 3: Problém hodujúcich filozofov — SEKVENČNÁ verzia (C++)
// ============================================================================
// Kompilácia: g++ -O2 -o main_seq src/main_seq.cpp
// Spustenie:  ./main_seq [počet_filozofov] [počet_jedál] [intenzita]
// ============================================================================

#include "dining.h"

int main(int argc, char* argv[]) {
    std::vector<DiningConfig> configs = {
        {5,  10, 500, 500},
        {5,  50, 500, 500},
        {10, 10, 500, 500},
        {10, 50, 500, 500},
    };

    if (argc >= 3) {
        int np = std::stoi(argv[1]);
        int nm = std::stoi(argv[2]);
        int intensity = (argc >= 4) ? std::stoi(argv[3]) : 500;
        configs = {{np, nm, intensity, intensity}};
    }

    for (const auto& cfg : configs) {
        std::cout << "\n=== Sekvenčné hodovanie: " << cfg.num_philosophers << " filozofov, "
                  << cfg.num_meals << " jedál ===" << std::endl;

        std::vector<PhilosopherStats> stats;
        double time_ms = measure_time([&]() {
            dining_sequential(cfg, stats);
        });

        print_dining_stats(stats);
        check_fairness(stats);
        bool correct = verify_results(stats, cfg.num_meals);
        std::cout << "Správnosť: " << (correct ? "OK" : "CHYBA!") << std::endl;
        std::cout << "Celkový čas: " << std::fixed << std::setprecision(2) << time_ms << " ms" << std::endl;
    }

    return 0;
}
