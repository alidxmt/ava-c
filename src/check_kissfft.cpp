#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
extern "C" {
#include "kiss_fft.h"
}

int main() {
    const int N = 8;
    kiss_fft_cfg cfg = kiss_fft_alloc(N, 0, nullptr, nullptr);
    if (!cfg) { std::cerr << "kiss_fft_alloc failed\n"; return 1; }

    std::vector<kiss_fft_cpx> in(N), out(N);
    for (int n = 0; n < N; ++n) {
        double ang = 2.0 * M_PI * 1 * n / N; // a pure tone at bin 1
        in[n].r = (kiss_fft_scalar)std::cos(ang);
        in[n].i = (kiss_fft_scalar)std::sin(ang);
    }
    kiss_fft(cfg, in.data(), out.data());
    free(cfg);

    double mag1 = std::sqrt(out[1].r * out[1].r + out[1].i * out[1].i);
    std::cout << "KissFFT OK. N=" << N << ", bin1 magnitude=" << mag1 << "\n";
    return 0;
}
