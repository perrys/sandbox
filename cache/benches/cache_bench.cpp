
#include <benchmark/benchmark.h>

#include <chrono>
#include <cstdio>
#include <iostream>

namespace {
using INT_TYPE = uint8_t;

constexpr size_t BUF_SIZE = 1024 * 1024 * 1024;

alignas(64) INT_TYPE buffer[BUF_SIZE];
alignas(64) INT_TYPE result[BUF_SIZE];

} // namespace

size_t square(INT_TYPE* dest, INT_TYPE* src, size_t nElements, size_t skip) {
    size_t count = 0;
    for (size_t i = 1; i < nElements; i += skip) {
        dest[i] = src[i] * src[i - 1];
        ++count;
    }
    return count;
}

size_t times3(INT_TYPE* src, size_t nElements, size_t skip) {
    size_t count = 0;
    for (size_t i = 0; i < nElements; i += skip) {
        src[i] *= 3;
        ++count;
    }
    return count;
}

size_t modifyCache(size_t len) {
    constexpr const size_t cacheLineHit = 64 / sizeof(INT_TYPE);

    int steps = 64 * 1024 * 1024; // Arbitrary number of steps
    size_t count = 0;

    for (int i = 0; i < steps; i++) {
        buffer[(i * cacheLineHit) % len]++;
        ++count;
    }
    return count;
}

static void cacheSizeBM(benchmark::State& state) {
    size_t len = static_cast<size_t>(state.range(0));

    for (size_t i = 0; i < BUF_SIZE; ++i) {
        buffer[i] = i + 1;
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(modifyCache(len));
    }
}
BENCHMARK(cacheSizeBM)->RangeMultiplier(2)->Range(1, BUF_SIZE);

static void DISABLED_cacheLineBM(benchmark::State& state) {
    size_t skip = static_cast<size_t>(state.range(0));

    for (size_t i = 0; i < BUF_SIZE; ++i) {
        buffer[i] = i + 1;
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(times3(buffer, BUF_SIZE, skip));
    }
}
BENCHMARK(DISABLED_cacheLineBM)->DenseRange(1, 40, 1);

#if 0
BENCHMARK_MAIN();
#else

int main(int argc, char* argv[]) {

    size_t skip = atoi(argv[1]);
    size_t nrepeat = argc > 2 ? atoi(argv[2]) : 1;

    for (size_t n = 0; n < nrepeat; ++n) {
        for (size_t i = 0; i < BUF_SIZE; ++i) {
            buffer[i] = i + 1;
        }
    }

    auto t0 = std::chrono::system_clock::now();
    times3(buffer, BUF_SIZE, skip);
    auto t1 = std::chrono::system_clock::now();
    auto dt = t1 - t0;
    std::cout << skip << ", " << dt << std::endl;
}
#endif
