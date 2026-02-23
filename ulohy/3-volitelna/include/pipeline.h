#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <map>

// ============================================================================
// Pomocné funkcie pre pipeline spracovanie dát (Úloha 3)
// ============================================================================

/// Štruktúra záznamu (log entry)
struct LogRecord {
    int id;
    std::string timestamp;
    std::string level;       // INFO, WARN, ERROR
    double value;
    std::string message;
};

/// Výsledok spracovania
struct ProcessedRecord {
    int id;
    std::string level;
    double transformed_value;
    bool is_anomaly;
};

/// Štatistiky (zdieľané medzi vláknami — vyžaduje synchronizáciu)
struct Stats {
    long long total_count = 0;
    long long error_count = 0;
    long long warn_count = 0;
    long long anomaly_count = 0;
    double sum_values = 0.0;
    double max_value = -1e18;
    double min_value = 1e18;
};

/// Generovanie testovacích záznamov
inline std::vector<LogRecord> generate_records(int count) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> level_dist(0, 9);
    std::normal_distribution<double> value_dist(50.0, 15.0);
    std::uniform_int_distribution<int> anomaly_dist(0, 99);

    std::vector<LogRecord> records(count);
    for (int i = 0; i < count; ++i) {
        int lv = level_dist(rng);
        std::string level = lv < 7 ? "INFO" : (lv < 9 ? "WARN" : "ERROR");

        double val = value_dist(rng);
        // Občas vložíme anomáliu
        if (anomaly_dist(rng) < 3) val *= 10.0;

        records[i] = {i, "2026-02-23T12:00:00Z", level, val,
                       "Record #" + std::to_string(i)};
    }
    return records;
}

/// Spracovanie jedného záznamu (výpočtovo náročná transformácia)
inline ProcessedRecord process_record(const LogRecord& rec) {
    // Simulácia výpočtovej záťaže
    double val = rec.value;
    for (int i = 0; i < 100; ++i) {
        val = std::sin(val) * std::cos(val) + std::sqrt(std::abs(val) + 1.0);
    }

    bool is_anomaly = std::abs(rec.value) > 100.0;
    return {rec.id, rec.level, val, is_anomaly};
}

/// Aktualizácia štatistík (bez synchronizácie — pre sekvenčnú verziu)
inline void update_stats(Stats& stats, const ProcessedRecord& rec) {
    stats.total_count++;
    if (rec.level == "ERROR") stats.error_count++;
    if (rec.level == "WARN") stats.warn_count++;
    if (rec.is_anomaly) stats.anomaly_count++;
    stats.sum_values += rec.transformed_value;
    stats.max_value = std::max(stats.max_value, rec.transformed_value);
    stats.min_value = std::min(stats.min_value, rec.transformed_value);
}

/// Výpis štatistík
inline void print_stats(const Stats& stats) {
    std::cout << "--- Štatistiky ---" << std::endl;
    std::cout << "Celkom záznamov: " << stats.total_count << std::endl;
    std::cout << "Chýb (ERROR):    " << stats.error_count << std::endl;
    std::cout << "Varovaní (WARN): " << stats.warn_count << std::endl;
    std::cout << "Anomálií:        " << stats.anomaly_count << std::endl;
    std::cout << "Priemer hodnôt:  " << std::fixed << std::setprecision(4)
              << (stats.total_count > 0 ? stats.sum_values / stats.total_count : 0) << std::endl;
    std::cout << "Min/Max:         " << stats.min_value << " / " << stats.max_value << std::endl;
}

/// Meranie času
template <typename Func>
double measure_time(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}
