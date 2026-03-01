// ============================================================================
// Úloha 2: K-means klasterizácia — PARALELNÁ verzia (Intel TBB)
// ============================================================================
// Kompilácia: g++ -O2 -o main_tbb src/main_tbb.cpp -ltbb
// Spustenie:  ./main_tbb [počet_bodov] [počet_zhlukov] [iterácie] [vlákna]
// ============================================================================
//
// Paralelizácia:
//   - tbb::parallel_for na priradenie bodov k centroidom (assignment step)
//   - tbb::combinable na thread-local akumuláciu centroidov (reduce step)
//   - Každá iterácia K-means: parallel assign → parallel reduce → update
// ============================================================================

#include "kmeans.h"
#include <tbb/tbb.h>
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/combinable.h>
#include <tbb/blocked_range.h>

/// Paralelné priradenie bodov k centroidom (TBB parallel_for)
std::vector<int> assign_clusters_tbb(const std::vector<Point>& points,
                                      const std::vector<Point>& centroids) {
    int n = points.size();
    int k = centroids.size();
    std::vector<int> assignments(n);

    tbb::parallel_for(
        tbb::blocked_range<int>(0, n),
        [&](const tbb::blocked_range<int>& r) {
            for (int i = r.begin(); i < r.end(); ++i) {
                double min_dist = std::numeric_limits<double>::max();
                int best = 0;
                for (int j = 0; j < k; ++j) {
                    double d = distance_sq(points[i], centroids[j]);
                    if (d < min_dist) {
                        min_dist = d;
                        best = j;
                    }
                }
                assignments[i] = best;
            }
        }
    );

    return assignments;
}

/// Paralelný výpočet nových centroidov (TBB combinable + parallel_for)
std::vector<Point> compute_centroids_tbb(const std::vector<Point>& points,
                                          const std::vector<int>& assignments,
                                          int k) {
    // Thread-local akumulátory pomocou tbb::combinable
    struct Accum {
        std::vector<double> sum_x, sum_y;
        std::vector<int> counts;
        Accum() = default;
        Accum(int k) : sum_x(k, 0.0), sum_y(k, 0.0), counts(k, 0) {}
    };

    tbb::combinable<Accum> local_accum([k]() { return Accum(k); });

    tbb::parallel_for(
        tbb::blocked_range<int>(0, (int)points.size()),
        [&](const tbb::blocked_range<int>& r) {
            auto& acc = local_accum.local();
            for (int i = r.begin(); i < r.end(); ++i) {
                int c = assignments[i];
                acc.sum_x[c] += points[i].x;
                acc.sum_y[c] += points[i].y;
                acc.counts[c]++;
            }
        }
    );

    // Kombinácia výsledkov z jednotlivých vlákien
    Accum combined(k);
    local_accum.combine_each([&](const Accum& acc) {
        for (int j = 0; j < k; ++j) {
            combined.sum_x[j] += acc.sum_x[j];
            combined.sum_y[j] += acc.sum_y[j];
            combined.counts[j] += acc.counts[j];
        }
    });

    std::vector<Point> centroids(k);
    for (int j = 0; j < k; ++j) {
        if (combined.counts[j] > 0) {
            centroids[j] = {combined.sum_x[j] / combined.counts[j],
                            combined.sum_y[j] / combined.counts[j]};
        }
    }
    return centroids;
}

/// Paralelný K-means algoritmus (TBB)
std::vector<int> kmeans_tbb(const std::vector<Point>& points, int k, int max_iter = 100) {
    auto centroids = init_centroids(points, k);
    std::vector<int> assignments;

    for (int iter = 0; iter < max_iter; ++iter) {
        assignments = assign_clusters_tbb(points, centroids);
        auto new_centroids = compute_centroids_tbb(points, assignments, k);

        // Konvergencia?
        double shift = 0.0;
        for (int j = 0; j < k; ++j) {
            shift += distance_sq(centroids[j], new_centroids[j]);
        }
        centroids = new_centroids;
        if (shift < 1e-10) break;
    }

    return assignments;
}

int main(int argc, char* argv[]) {
    int num_points = 1000000, num_clusters = 10, max_iter = 50;
    std::vector<int> thread_counts = {2, 4, 8};

    if (argc >= 3) {
        num_points = std::stoi(argv[1]);
        num_clusters = std::stoi(argv[2]);
    }
    if (argc >= 4) max_iter = std::stoi(argv[3]);
    if (argc >= 5) thread_counts = {std::stoi(argv[4])};

    auto points = generate_data(num_points, num_clusters);

    // --- Sekvenčná verzia pre porovnanie ---
    std::vector<int> result_seq;
    double time_seq = measure_time([&]() {
        result_seq = kmeans_seq(points, num_clusters, max_iter);
    });
    std::cout << "Sekvenčný čas: " << std::fixed << std::setprecision(2) << time_seq << " ms" << std::endl;
    print_kmeans_stats(result_seq, num_clusters);

    // --- TBB paralelná verzia ---
    for (int t : thread_counts) {
        tbb::global_control gc(tbb::global_control::max_allowed_parallelism, t);

        std::cout << "\n=== TBB K-means: " << num_points << " bodov, "
                  << num_clusters << " zhlukov, " << t << " vlákien ===" << std::endl;

        std::vector<int> result_tbb;
        double time_tbb = measure_time([&]() {
            result_tbb = kmeans_tbb(points, num_clusters, max_iter);
        });

        bool correct = assignments_equal(result_seq, result_tbb);
        double speedup = time_seq / time_tbb;

        print_kmeans_stats(result_tbb, num_clusters);
        std::cout << "TBB čas: " << std::fixed << std::setprecision(2) << time_tbb << " ms" << std::endl;
        std::cout << "Správnosť: " << (correct ? "OK" : "ODLIŠNÉ (rôzne poradie operácií)") << std::endl;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    }

    return 0;
}
