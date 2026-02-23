// ============================================================================
// Úloha 3: Producer-Consumer Pipeline — SEKVENČNÁ verzia (C++)
// ============================================================================
// Kompilácia: g++ -O2 -o main_seq src/main_seq.cpp
// Spustenie:  ./main_seq [počet_záznamov]
// ============================================================================

#include "pipeline.h"

int main(int argc, char* argv[]) {
    std::vector<int> record_counts = {100000, 1000000, 10000000};

    if (argc >= 2) {
        record_counts = {std::stoi(argv[1])};
    }

    for (int count : record_counts) {
        std::cout << "\n=== Sekvenčný pipeline — " << count << " záznamov ===" << std::endl;

        // Fáza 1: Generovanie (simulácia čítania)
        auto records = generate_records(count);

        Stats stats;
        std::vector<ProcessedRecord> results;
        results.reserve(count);

        double time_ms = measure_time([&]() {
            for (const auto& rec : records) {
                // Fáza 2: Spracovanie
                auto processed = process_record(rec);

                // Fáza 3: Aktualizácia štatistík a uloženie
                update_stats(stats, processed);
                results.push_back(processed);
            }
        });

        print_stats(stats);
        std::cout << "Čas: " << std::fixed << std::setprecision(2) << time_ms << " ms" << std::endl;
    }

    return 0;
}
