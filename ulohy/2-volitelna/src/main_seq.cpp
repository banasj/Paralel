// ============================================================================
// Úloha 2: Gaussovský filter — SEKVENČNÁ verzia (C++)
// ============================================================================
// Kompilácia: g++ -O2 -o main_seq src/main_seq.cpp
// Spustenie:  ./main_seq [šírka] [výška] [veľkosť_jadra]
// ============================================================================

#include "image_filter.h"

int main(int argc, char* argv[]) {
    // Defaultné parametre
    struct Config {
        int width, height, kernel_size;
    };
    std::vector<Config> configs = {
        {1920, 1080, 5},
        {1920, 1080, 11},
        {1920, 1080, 21},
        {3840, 2160, 5},
        {3840, 2160, 11},
        {3840, 2160, 21},
    };

    if (argc >= 4) {
        configs = {{std::stoi(argv[1]), std::stoi(argv[2]), std::stoi(argv[3])}};
    }

    for (const auto& cfg : configs) {
        std::cout << "\n=== Sekvenčný Gaussian blur " << cfg.width << "x" << cfg.height
                  << " (jadro " << cfg.kernel_size << "x" << cfg.kernel_size << ") ===" << std::endl;

        auto img = generate_test_image(cfg.width, cfg.height);
        auto kernel = generate_gaussian_kernel(cfg.kernel_size);

        Image result;
        double time_ms = measure_time([&]() {
            result = apply_filter_seq(img, kernel);
        });

        std::cout << "Čas: " << std::fixed << std::setprecision(2) << time_ms << " ms" << std::endl;
    }

    return 0;
}
