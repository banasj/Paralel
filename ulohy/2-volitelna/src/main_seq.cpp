// ============================================================================
// Úloha 2: K-means klasterizácia — SEKVENČNÁ verzia (C++)
// ============================================================================
// Kompilácia: g++ -O2 -o main_seq src/main_seq.cpp
// Spustenie:  ./main_seq [počet_bodov] [počet_zhlukov] [iterácie]
// ============================================================================

#include "kmeans.h"

int main(int argc, char* argv[]) {
    struct Config {
        int num_points, num_clusters, max_iter;
    };
    std::vector<Config> configs = {
        {100000,  5,  50},
        {100000,  10, 50},
        {1000000, 5,  50},
        {1000000, 10, 50},
        {1000000, 20, 50},
    };

    if (argc >= 3) {
        int np = std::stoi(argv[1]);
        int nc = std::stoi(argv[2]);
        int mi = (argc >= 4) ? std::stoi(argv[3]) : 50;
        configs = {{np, nc, mi}};
    }

    for (const auto& cfg : configs) {
        std::cout << "\n=== Sekvenčný K-means: " << cfg.num_points << " bodov, "
                  << cfg.num_clusters << " zhlukov, max " << cfg.max_iter << " iterácií ===" << std::endl;

        auto points = generate_data(cfg.num_points, cfg.num_clusters);

        std::vector<int> assignments;
        double time_ms = measure_time([&]() {
            assignments = kmeans_seq(points, cfg.num_clusters, cfg.max_iter);
        });

        print_kmeans_stats(assignments, cfg.num_clusters);
        std::cout << "Čas: " << std::fixed << std::setprecision(2) << time_ms << " ms" << std::endl;
    }

    return 0;
}
