// ============================================================================
// Úloha 3: Problém hodujúcich filozofov — PARALELNÁ verzia (Intel TBB)
// ============================================================================
// Kompilácia: g++ -O2 -o main_tbb src/main_tbb.cpp -ltbb
// Spustenie:  ./main_tbb [počet_filozofov] [počet_jedál] [intenzita] [vlákna]
//             ./main_tbb --no-sync   ← spustí iba verziu BEZ synchronizácie
//             ./main_tbb --sync      ← spustí iba verziu SO synchronizáciou
//             (bez flagu sa spustia OBE verzie a porovnajú sa)
// ============================================================================
//
// Synchronizácia:
//   - tbb::mutex pre vidličky (zdieľané prostriedky medzi susednými filozofmi)
//   - Usporiadanie zdrojov (resource ordering) na prevenciu deadlocku:
//     každý filozof najprv zamkne vidličku s nižším indexom
//   - tbb::parallel_for na paralelné spustenie všetkých filozofov
//
// Demonštrácia problémov BEZ synchronizácie:
//   1. SHARED COUNTER (total_meals_shared) — int, nie atomic:
//      - S mutexom: presne N * num_meals (chránený v kritickom úseku)
//      - Bez mutexu: < N * num_meals (lost updates kvôli data race)
//   2. FORK VIOLATIONS — sledovanie simultánneho prístupu k vidličkám:
//      - S mutexom: 0 violations (exkluzívny prístup garantovaný)
//      - Bez mutexu: > 0 violations (dvaja filozofi "držia" tú istú vidličku)
//
// Race condition bez synchronizácie:
//   - Dvaja susední filozofi môžu simultánne zdvihnúť rovnakú vidličku
//   - Bez mutexu: data race na zdieľanom stave vidličky → undefined behavior
//   - Bez usporiadania: potenciálny deadlock (všetci držia ľavú, čakajú na pravú)
// ============================================================================

#include "dining.h"
#include <tbb/tbb.h>
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/mutex.h>

/// Paralelná simulácia — filozofi jedia súčasne
/// mode == SYNC_ORDERED: mutex + resource ordering (korektné)
/// mode == SYNC_NONE:    žiadne mutexy (data race — úmyselne na demonštráciu)
DiningResult dining_tbb(const DiningConfig& cfg, SyncMode mode) {
    int n = cfg.num_philosophers;
    DiningResult result;
    result.stats.resize(n);
    for (int i = 0; i < n; ++i) result.stats[i].id = i;

    // Vidličky — zdieľané prostriedky chránené mutexami (iba v SYNC_ORDERED)
    std::vector<tbb::mutex> forks(n);

    // Sledovanie fork violations — atomic<int> pre bezpečné sledovanie
    // (samotné sledovanie musí byť thread-safe, inak nemôžeme spoľahlivo počítať)
    std::vector<std::atomic<int>> fork_usage(n);
    for (int i = 0; i < n; ++i) fork_usage[i].store(0);
    std::atomic<int> fork_violations{0};

    // Zdieľaný counter jedál — ÚMYSELNE int (nie atomic!)
    // Bez mutexu: concurrent read-modify-write → lost updates
    // Toto je kľúčový dôkaz, prečo je synchronizácia nevyhnutná
    int total_meals_shared = 0;

    tbb::parallel_for(0, n, [&](int i) {
        for (int meal = 0; meal < cfg.num_meals; ++meal) {
            // ---- MYSLENIE ----
            auto think_start = std::chrono::high_resolution_clock::now();
            simulate_work(cfg.think_intensity);
            auto think_end = std::chrono::high_resolution_clock::now();
            result.stats[i].total_thinking_ms +=
                std::chrono::duration<double, std::milli>(think_end - think_start).count();

            // ---- ZDVIHNUTIE VIDLIČIEK ----
            int left = i;
            int right = (i + 1) % n;

            if (mode == SYNC_ORDERED) {
                // Usporiadanie zdrojov — prevencia deadlocku:
                // Vždy zamykáme vidličku s nižším indexom ako prvú.
                // Bez toho: ak každý filozof zamkne ľavú vidličku prvú,
                // všetci čakajú na pravú → DEADLOCK!
                int first  = std::min(left, right);
                int second = std::max(left, right);

                auto wait_start = std::chrono::high_resolution_clock::now();
                tbb::mutex::scoped_lock lock1(forks[first]);
                tbb::mutex::scoped_lock lock2(forks[second]);
                auto wait_end = std::chrono::high_resolution_clock::now();
                result.stats[i].total_waiting_ms +=
                    std::chrono::duration<double, std::milli>(wait_end - wait_start).count();

                // Sledovanie fork usage (vždy OK s mutexom)
                fork_usage[left].fetch_add(1);
                fork_usage[right].fetch_add(1);
                if (fork_usage[left].load() > 1 || fork_usage[right].load() > 1) {
                    fork_violations.fetch_add(1);
                }

                // ---- JEDENIE (s oboma vidličkami, chránené mutexom) ----
                auto eat_start = std::chrono::high_resolution_clock::now();
                simulate_work(cfg.eat_intensity);
                auto eat_end = std::chrono::high_resolution_clock::now();
                result.stats[i].total_eating_ms +=
                    std::chrono::duration<double, std::milli>(eat_end - eat_start).count();
                result.stats[i].meals_eaten++;

                // Zdieľaný counter — chránený mutexom → žiadne lost updates
                total_meals_shared++;

                fork_usage[left].fetch_sub(1);
                fork_usage[right].fetch_sub(1);

                // Vidličky sa automaticky uvoľnia (scoped_lock destruktor)

            } else {
                // SYNC_NONE — žiadne mutexy
                // Data race na vidličkách aj zdieľanom countri!

                auto wait_start = std::chrono::high_resolution_clock::now();
                // (žiadne zamykanie — okamžitý "prístup")
                auto wait_end = std::chrono::high_resolution_clock::now();
                result.stats[i].total_waiting_ms +=
                    std::chrono::duration<double, std::milli>(wait_end - wait_start).count();

                // Sledovanie fork violations — bez mutexu dvaja filozofi
                // môžu "držať" tú istú vidličku súčasne
                fork_usage[left].fetch_add(1);
                fork_usage[right].fetch_add(1);
                if (fork_usage[left].load() > 1 || fork_usage[right].load() > 1) {
                    fork_violations.fetch_add(1);
                }

                // ---- JEDENIE (BEZ ochrany — data race na vidličkách!) ----
                auto eat_start = std::chrono::high_resolution_clock::now();
                simulate_work(cfg.eat_intensity);
                auto eat_end = std::chrono::high_resolution_clock::now();
                result.stats[i].total_eating_ms +=
                    std::chrono::duration<double, std::milli>(eat_end - eat_start).count();
                result.stats[i].meals_eaten++;

                // Zdieľaný counter — NECHRÁNENÝ → lost updates (data race)
                // Viac vlákien číta rovnakú hodnotu, inkrementuje, zapíše → strata
                total_meals_shared++;

                fork_usage[left].fetch_sub(1);
                fork_usage[right].fetch_sub(1);
            }
        }
    });

    result.total_meals_shared = total_meals_shared;
    result.fork_violations = fork_violations.load();
    return result;
}

int main(int argc, char* argv[]) {
    int num_philosophers = 5, num_meals = 50;
    int intensity = 500;
    std::vector<int> thread_counts = {2, 4, 8};
    bool run_sync = true, run_nosync = true;

    // Parsovanie CLI argumentov
    for (int a = 1; a < argc; ++a) {
        std::string arg = argv[a];
        if (arg == "--no-sync")  { run_sync = false; run_nosync = true; continue; }
        if (arg == "--sync")     { run_sync = true;  run_nosync = false; continue; }
    }
    // Pozičné argumenty: [filozofi] [jedlá] [intenzita] [vlákna]
    std::vector<std::string> pos_args;
    for (int a = 1; a < argc; ++a) {
        std::string arg = argv[a];
        if (arg[0] != '-') pos_args.push_back(arg);
    }
    if (pos_args.size() >= 1) num_philosophers = std::stoi(pos_args[0]);
    if (pos_args.size() >= 2) num_meals = std::stoi(pos_args[1]);
    if (pos_args.size() >= 3) intensity = std::stoi(pos_args[2]);
    if (pos_args.size() >= 4) thread_counts = {std::stoi(pos_args[3])};

    DiningConfig cfg = {num_philosophers, num_meals, intensity, intensity};
    int expected_total = num_philosophers * num_meals;

    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Problém hodujúcich filozofov — Porovnanie synchronizácie   ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  Filozofi: " << std::setw(4) << num_philosophers
              << "   Jedlá: " << std::setw(4) << num_meals
              << "   Intenzita: " << std::setw(5) << intensity << "       ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;

    // --- Sekvenčná verzia pre baseline ---
    std::vector<PhilosopherStats> stats_seq;
    double time_seq = measure_time([&]() {
        dining_sequential(cfg, stats_seq);
    });

    std::cout << "\n========================================" << std::endl;
    std::cout << "  SEKVENČNÁ VERZIA (baseline)" << std::endl;
    std::cout << "========================================" << std::endl;
    print_dining_stats(stats_seq);
    bool seq_correct = verify_results(stats_seq, num_meals);
    std::cout << "Správnosť: " << (seq_correct ? "OK" : "CHYBA!") << std::endl;
    std::cout << "Celkový čas: " << std::fixed << std::setprecision(2) << time_seq << " ms" << std::endl;

    // --- TBB paralelná verzia pre každý počet vlákien ---
    for (int num_threads : thread_counts) {
        tbb::global_control gc(tbb::global_control::max_allowed_parallelism, num_threads);

        std::cout << "\n╔══════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  TBB: " << num_philosophers << " filozofov, "
                  << num_meals << " jedál, " << num_threads << " vlákien" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════╝" << std::endl;

        DiningResult sync_result, nosync_result;

        // 1. Verzia SO synchronizáciou (SYNC_ORDERED)
        if (run_sync) {
            std::cout << "\n--- Režim: SYNC_ORDERED (mutex + resource ordering) ---" << std::endl;
            sync_result.time_ms = measure_time([&]() {
                sync_result = dining_tbb(cfg, SYNC_ORDERED);
            });
            print_dining_stats(sync_result.stats);
            check_fairness(sync_result.stats);
            verify_sync_results(sync_result, num_meals, num_philosophers);
            std::cout << "TBB čas: " << std::fixed << std::setprecision(2)
                      << sync_result.time_ms << " ms" << std::endl;
            std::cout << "Speedup vs sekvenčná: " << std::setprecision(2)
                      << (time_seq / sync_result.time_ms) << "x" << std::endl;
        }

        // 2. Verzia BEZ synchronizácie (SYNC_NONE)
        if (run_nosync) {
            std::cout << "\n--- Režim: SYNC_NONE (bez mutexov — data race!) ---" << std::endl;
            nosync_result.time_ms = measure_time([&]() {
                nosync_result = dining_tbb(cfg, SYNC_NONE);
            });
            print_dining_stats(nosync_result.stats);
            check_fairness(nosync_result.stats);
            verify_sync_results(nosync_result, num_meals, num_philosophers);
            std::cout << "TBB čas: " << std::fixed << std::setprecision(2)
                      << nosync_result.time_ms << " ms" << std::endl;
            std::cout << "Speedup vs sekvenčná: " << std::setprecision(2)
                      << (time_seq / nosync_result.time_ms) << "x" << std::endl;
        }

        // 3. Porovnávacia tabuľka (ak obe verzie bežali)
        if (run_sync && run_nosync) {
            print_comparison(sync_result, nosync_result, time_seq, expected_total);
        }
    }

    return 0;
}
