// ============================================================================
// Úloha 3: Producer-Consumer Pipeline — PARALELNÁ verzia (Intel TBB)
// ============================================================================
// Kompilácia: g++ -O2 -o main_tbb src/main_tbb.cpp -ltbb
// Spustenie:  ./main_tbb [počet_záznamov] [vlákna]
// ============================================================================
//
// Synchronizácia:
//   - tbb::mutex na ochranu zdieľaných štatistík (Stats)
//   - tbb::concurrent_vector na thread-safe ukladanie výsledkov
//   - tbb::parallel_pipeline s serial/parallel filtrami
// ============================================================================

#include "pipeline.h"
#include <tbb/tbb.h>
#include <tbb/global_control.h>
#include <tbb/parallel_pipeline.h>
#include <tbb/mutex.h>
#include <tbb/concurrent_vector.h>
#include <atomic>

int main(int argc, char* argv[]) {
    int record_count = 1000000;
    std::vector<int> thread_counts = {2, 4, 8};

    if (argc >= 2) record_count = std::stoi(argv[1]);
    if (argc >= 3) thread_counts = {std::stoi(argv[2])};

    // Generovanie záznamov
    auto records = generate_records(record_count);

    // --- Sekvenčná verzia pre porovnanie ---
    Stats stats_seq;
    std::vector<ProcessedRecord> results_seq;
    results_seq.reserve(record_count);

    double time_seq = measure_time([&]() {
        for (const auto& rec : records) {
            auto processed = process_record(rec);
            update_stats(stats_seq, processed);
            results_seq.push_back(processed);
        }
    });

    std::cout << "=== Sekvenčný pipeline — " << record_count << " záznamov ===" << std::endl;
    print_stats(stats_seq);
    std::cout << "Čas: " << std::fixed << std::setprecision(2) << time_seq << " ms\n" << std::endl;

    // --- TBB paralelná verzia ---
    for (int num_threads : thread_counts) {
        tbb::global_control gc(tbb::global_control::max_allowed_parallelism, num_threads);

        std::cout << "=== TBB pipeline — " << record_count << " záznamov, "
                  << num_threads << " vlákien ===" << std::endl;

        Stats stats_tbb;
        tbb::mutex stats_mutex;  // ← SYNCHRONIZÁCIA: mutex na ochranu zdieľaných štatistík
        tbb::concurrent_vector<ProcessedRecord> results_tbb;  // ← thread-safe kontajner
        std::atomic<int> read_index{0};

        double time_tbb = measure_time([&]() {
            tbb::parallel_pipeline(
                num_threads * 4,  // max_number_of_live_tokens

                // Filter 1: Čítanie záznamov (serial — zachovanie poradia)
                tbb::make_filter<void, const LogRecord*>(
                    tbb::filter_mode::serial_in_order,
                    [&](tbb::flow_control& fc) -> const LogRecord* {
                        int idx = read_index.fetch_add(1);
                        if (idx >= record_count) {
                            fc.stop();
                            return nullptr;
                        }
                        return &records[idx];
                    }
                ) &

                // Filter 2: Spracovanie (parallel — hlavná výpočtová záťaž)
                tbb::make_filter<const LogRecord*, ProcessedRecord>(
                    tbb::filter_mode::parallel,
                    [&](const LogRecord* rec) -> ProcessedRecord {
                        return process_record(*rec);
                    }
                ) &

                // Filter 3: Zápis výsledkov a aktualizácia štatistík (serial — synchronizácia)
                tbb::make_filter<ProcessedRecord, void>(
                    tbb::filter_mode::serial_in_order,
                    [&](ProcessedRecord processed) {
                        // Synchronizovaný prístup k štatistikám
                        {
                            tbb::mutex::scoped_lock lock(stats_mutex);
                            update_stats(stats_tbb, processed);
                        }
                        results_tbb.push_back(processed);
                    }
                )
            );
        });

        print_stats(stats_tbb);
        double speedup = time_seq / time_tbb;
        std::cout << "Čas: " << std::fixed << std::setprecision(2) << time_tbb << " ms" << std::endl;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;

        // Overenie správnosti
        bool count_match = (stats_seq.total_count == stats_tbb.total_count);
        std::cout << "Správnosť (počet): " << (count_match ? "OK" : "CHYBA!") << std::endl;
        std::cout << std::endl;
    }

    return 0;
}
