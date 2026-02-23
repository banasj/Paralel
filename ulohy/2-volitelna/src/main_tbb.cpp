// ============================================================================
// Úloha 2: Gaussovský filter — PARALELNÁ verzia (Intel TBB)
// ============================================================================
// Kompilácia: g++ -O2 -o main_tbb src/main_tbb.cpp -ltbb
// Spustenie:  ./main_tbb [šírka] [výška] [veľkosť_jadra] [vlákna]
// ============================================================================

#include "image_filter.h"
#include <tbb/tbb.h>
#include <tbb/global_control.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>

/// Paralelná aplikácia konvolučného filtra pomocou TBB
Image apply_filter_tbb(const Image& input, const std::vector<std::vector<float>>& kernel) {
    int ksize = kernel.size();
    int half = ksize / 2;
    Image output(input.width, input.height);

    tbb::parallel_for(
        tbb::blocked_range2d<int>(0, input.height, 0, input.width),
        [&](const tbb::blocked_range2d<int>& r) {
            for (int row = r.rows().begin(); row < r.rows().end(); ++row) {
                for (int col = r.cols().begin(); col < r.cols().end(); ++col) {
                    float sum = 0.0f;
                    for (int ki = -half; ki <= half; ++ki) {
                        for (int kj = -half; kj <= half; ++kj) {
                            int rr = std::min(std::max(row + ki, 0), input.height - 1);
                            int cc = std::min(std::max(col + kj, 0), input.width - 1);
                            sum += input.at(rr, cc) * kernel[ki + half][kj + half];
                        }
                    }
                    output.at(row, col) = sum;
                }
            }
        }
    );

    return output;
}

int main(int argc, char* argv[]) {
    int width = 1920, height = 1080, ksize = 11;
    std::vector<int> thread_counts = {2, 4, 8};

    if (argc >= 4) {
        width = std::stoi(argv[1]);
        height = std::stoi(argv[2]);
        ksize = std::stoi(argv[3]);
    }
    if (argc >= 5) {
        thread_counts = {std::stoi(argv[4])};
    }

    auto img = generate_test_image(width, height);
    auto kernel = generate_gaussian_kernel(ksize);

    // Sekvenčná verzia pre porovnanie
    Image result_seq;
    double time_seq = measure_time([&]() {
        result_seq = apply_filter_seq(img, kernel);
    });
    std::cout << "Sekvenčný čas: " << std::fixed << std::setprecision(2) << time_seq << " ms\n";

    for (int t : thread_counts) {
        tbb::global_control gc(tbb::global_control::max_allowed_parallelism, t);

        std::cout << "\n=== TBB Gaussian blur " << width << "x" << height
                  << " (jadro " << ksize << "x" << ksize << ", " << t << " vlákien) ===" << std::endl;

        Image result_tbb;
        double time_tbb = measure_time([&]() {
            result_tbb = apply_filter_tbb(img, kernel);
        });

        bool correct = images_equal(result_seq, result_tbb);
        double speedup = time_seq / time_tbb;

        std::cout << "TBB čas: " << std::fixed << std::setprecision(2) << time_tbb << " ms" << std::endl;
        std::cout << "Správnosť: " << (correct ? "OK" : "CHYBA!") << std::endl;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    }

    return 0;
}
