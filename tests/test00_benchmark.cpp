#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

#include <nanobench.h>

#include <random>
#include <functional>
#include <vector>

using namespace btx::memory;

TEST_CASE("Benchmarking") {
    std::random_device dev;
    std::mt19937 rng(dev());

    auto heap_size_rng = std::bind(
        std::uniform_int_distribution<std::size_t>(1 << 16, 1 << 24),
        rng
    );

    auto alloc_size_rng = std::bind(
        std::uniform_int_distribution<std::size_t>(1 << 4, 1 << 8),
        rng
    );

    std::vector<std::size_t> alloc_sizes;
    alloc_sizes.resize(1000);

    for(auto &alloc_size : alloc_sizes) {
        alloc_size = alloc_size_rng();
    }

    auto bench = ankerl::nanobench::Bench()
        .minEpochIterations(1000)
        .relative(true);

    bench.run(
        "libstdc malloc and free benchmark", [&] {
            for(auto const alloc_size : alloc_sizes) {
                void *ignored = ::malloc(alloc_size);
                ankerl::nanobench::doNotOptimizeAway(ignored);
                ::free(ignored);
            }
        }
    );

    Heap heap(heap_size_rng());
    bench.run(
        "btx::memory alloc and free benchmark", [&] {
            for(auto const alloc_size : alloc_sizes) {
                void *ignored = heap.alloc(alloc_size);
                heap.free(ignored);
            }
        }
    );
}