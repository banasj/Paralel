#pragma once

#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <numeric>
#include <algorithm>
#include <limits>

// ============================================================================
// Pomocné funkcie pre K-means klasterizáciu (Úloha 2)
// ============================================================================

/// 2D bod
struct Point {
    double x, y;
};

/// Euklidovská vzdialenosť^2
inline double distance_sq(const Point& a, const Point& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

/// Generovanie testovacích dát — náhodné zhluky
inline std::vector<Point> generate_data(int num_points, int num_clusters, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> center_dist(-100.0, 100.0);
    std::normal_distribution<double> noise(0.0, 5.0);

    // Generujeme náhodné centrá zhlukov
    std::vector<Point> centers(num_clusters);
    for (auto& c : centers) {
        c.x = center_dist(rng);
        c.y = center_dist(rng);
    }

    // Generujeme body okolo centier
    std::vector<Point> points(num_points);
    std::uniform_int_distribution<int> cluster_dist(0, num_clusters - 1);
    for (auto& p : points) {
        int ci = cluster_dist(rng);
        p.x = centers[ci].x + noise(rng);
        p.y = centers[ci].y + noise(rng);
    }
    return points;
}

/// Inicializácia centroidov (náhodný výber bodov)
inline std::vector<Point> init_centroids(const std::vector<Point>& points, int k, unsigned seed = 123) {
    std::mt19937 rng(seed);
    std::vector<int> indices(points.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    std::vector<Point> centroids(k);
    for (int i = 0; i < k; ++i) {
        centroids[i] = points[indices[i]];
    }
    return centroids;
}

/// Priradenie bodov k najbližšiemu centroidu (sekvenčne)
inline std::vector<int> assign_clusters_seq(const std::vector<Point>& points,
                                             const std::vector<Point>& centroids) {
    int n = points.size();
    int k = centroids.size();
    std::vector<int> assignments(n);

    for (int i = 0; i < n; ++i) {
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
    return assignments;
}

/// Výpočet nových centroidov (sekvenčne)
inline std::vector<Point> compute_centroids_seq(const std::vector<Point>& points,
                                                 const std::vector<int>& assignments,
                                                 int k) {
    std::vector<double> sum_x(k, 0.0), sum_y(k, 0.0);
    std::vector<int> counts(k, 0);

    for (size_t i = 0; i < points.size(); ++i) {
        int c = assignments[i];
        sum_x[c] += points[i].x;
        sum_y[c] += points[i].y;
        counts[c]++;
    }

    std::vector<Point> centroids(k);
    for (int j = 0; j < k; ++j) {
        if (counts[j] > 0) {
            centroids[j] = {sum_x[j] / counts[j], sum_y[j] / counts[j]};
        }
    }
    return centroids;
}

/// Sekvenčný K-means algoritmus (Lloyd's algorithm)
inline std::vector<int> kmeans_seq(const std::vector<Point>& points, int k, int max_iter = 100) {
    auto centroids = init_centroids(points, k);
    std::vector<int> assignments;

    for (int iter = 0; iter < max_iter; ++iter) {
        assignments = assign_clusters_seq(points, centroids);
        auto new_centroids = compute_centroids_seq(points, assignments, k);

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

/// Výpis štatistík klasterizácie
inline void print_kmeans_stats(const std::vector<int>& assignments, int k) {
    std::vector<int> counts(k, 0);
    for (int c : assignments) counts[c]++;
    std::cout << "--- Veľkosti zhlukov ---" << std::endl;
    for (int j = 0; j < k; ++j) {
        std::cout << "  Zhluk " << j << ": " << counts[j] << " bodov" << std::endl;
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

/// Porovnanie výsledkov
inline bool assignments_equal(const std::vector<int>& a, const std::vector<int>& b) {
    return a == b;
}
