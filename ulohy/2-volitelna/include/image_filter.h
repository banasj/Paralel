#pragma once

#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <stdexcept>

// ============================================================================
// Pomocné funkcie pre spracovanie obrázkov (Gaussovský filter)
// ============================================================================

/// Jednoduchá štruktúra pre grayscale obrázok
struct Image {
    int width;
    int height;
    std::vector<float> pixels;  // row-major, hodnoty 0.0 – 255.0

    Image() : width(0), height(0) {}
    Image(int w, int h) : width(w), height(h), pixels(w * h, 0.0f) {}

    float& at(int row, int col) { return pixels[row * width + col]; }
    const float& at(int row, int col) const { return pixels[row * width + col]; }
};

/// Generovanie testovacieho obrázka s náhodnými hodnotami
inline Image generate_test_image(int width, int height) {
    Image img(width, height);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 255.0f);
    for (auto& p : img.pixels) p = dist(rng);
    return img;
}

/// Generovanie Gaussovského jadra
inline std::vector<std::vector<float>> generate_gaussian_kernel(int size, float sigma = 0.0f) {
    if (size % 2 == 0) size++;  // musí byť nepárne
    if (sigma <= 0.0f) sigma = size / 6.0f;

    std::vector<std::vector<float>> kernel(size, std::vector<float>(size));
    int half = size / 2;
    float sum = 0.0f;

    for (int i = -half; i <= half; ++i) {
        for (int j = -half; j <= half; ++j) {
            float val = std::exp(-(i * i + j * j) / (2.0f * sigma * sigma));
            kernel[i + half][j + half] = val;
            sum += val;
        }
    }
    // Normalizácia
    for (auto& row : kernel)
        for (auto& v : row)
            v /= sum;

    return kernel;
}

/// Sekvenčná aplikácia konvolučného filtra
inline Image apply_filter_seq(const Image& input, const std::vector<std::vector<float>>& kernel) {
    int ksize = kernel.size();
    int half = ksize / 2;
    Image output(input.width, input.height);

    for (int row = 0; row < input.height; ++row) {
        for (int col = 0; col < input.width; ++col) {
            float sum = 0.0f;
            for (int ki = -half; ki <= half; ++ki) {
                for (int kj = -half; kj <= half; ++kj) {
                    int r = std::min(std::max(row + ki, 0), input.height - 1);
                    int c = std::min(std::max(col + kj, 0), input.width - 1);
                    sum += input.at(r, c) * kernel[ki + half][kj + half];
                }
            }
            output.at(row, col) = sum;
        }
    }
    return output;
}

/// Meranie času
template <typename Func>
double measure_time(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

/// Porovnanie obrázkov
inline bool images_equal(const Image& a, const Image& b, float epsilon = 0.01f) {
    if (a.width != b.width || a.height != b.height) return false;
    for (size_t i = 0; i < a.pixels.size(); ++i) {
        if (std::abs(a.pixels[i] - b.pixels[i]) > epsilon) return false;
    }
    return true;
}
