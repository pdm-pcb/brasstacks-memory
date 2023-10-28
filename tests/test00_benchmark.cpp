#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

#include <nanobench.h>

#include <random>
#include <functional>
#include <vector>
#include <numeric>

using namespace btx::memory;

constexpr std::size_t alloc_count = 1000;
constexpr std::size_t min_epoch_count = 100;
constexpr std::size_t min_alloc_size = 1 << 4;
constexpr std::size_t max_alloc_size = 1 << 8;

TEST_CASE("Benchmarking") {
    // The first thing we need is a good ol' RNG on which to base our ranges
    std::random_device dev;
    std::default_random_engine rng(dev());

    // Next, a vector of random allocation sizes
    std::vector<std::size_t> alloc_sizes;
    alloc_sizes.resize(alloc_count);

    auto alloc_size_rng = std::bind(
        std::uniform_int_distribution<std::size_t>(min_alloc_size,
                                                   max_alloc_size),
        rng
    );

    for(auto &alloc_size : alloc_sizes) {
        alloc_size = alloc_size_rng();
    }

    // The benchmarks below will allocate 500 randomly sized blocks, then free
    // those 500 blocks in a random order, and repeat for the second 500 blocks.
    // In service of that, I'll make two vectors of indices (first half: 0-499,
    // second half 499-999) and shuffle them
    std::vector<std::size_t> free_order_first_half(alloc_count/2);
    std::iota(
        free_order_first_half.begin(),
        free_order_first_half.end(),
        0
    );
    std::shuffle(
        free_order_first_half.begin(),
        free_order_first_half.end(),
        rng
    );

    std::vector<std::size_t> free_order_second_half(alloc_count/2);
    std::iota(
        free_order_second_half.begin(),
        free_order_second_half.end(),
        alloc_count/2
    );
    std::shuffle(
        free_order_second_half.begin(),
        free_order_second_half.end(),
        rng
    );

    // Finally, a vector of pointers to store the allocations
    std::vector<void *> allocs(alloc_count);
    std::fill(allocs.begin(), allocs.end(), nullptr);

    // The benchmark object will run our lambdas at least 1000 times and
    // compare the results
    auto bench = ankerl::nanobench::Bench()
        .minEpochIterations(min_epoch_count)
        .relative(true);
//*
    // Test plain malloc() and free()
    bench.run(
        "libstdc malloc and free benchmark", [&] {
            std::size_t alloc = 0;

            // Allocate the first half
            do {
                allocs[alloc] = ::malloc(alloc_sizes[alloc]);

                // Zero the memory
                ::memset(allocs[alloc], 0, alloc_sizes[alloc]);

                // Write some "useful" information
                auto *new_block = static_cast<std::size_t *>(allocs[alloc]);
                *new_block = alloc_sizes[alloc];

                ++alloc;
            } while(alloc < alloc_count/2);

            // Free the first half in random order
            for(auto const index : free_order_first_half) {
                ::free(allocs[index]);
            }

            // Allocate the second half
            do {
                allocs[alloc] = ::malloc(alloc_sizes[alloc]);

                // Same nonosense as above
                ::memset(allocs[alloc], 0, alloc_sizes[alloc]);
                auto *new_block = static_cast<std::size_t *>(allocs[alloc]);
                *new_block = alloc_sizes[alloc];

                ++alloc;
            } while(alloc < alloc_count);

            // Free the second half in random order
            for(auto const index : free_order_second_half) {
                ::free(allocs[index]);
            }
        }
    );

    // Create a heap that's guaranteed to be able to hold all of our random
    // allocations
    auto heap_size_rng = std::bind(
        std::uniform_int_distribution<std::size_t>(
            max_alloc_size * alloc_count + 32u,
            max_alloc_size * alloc_count * 2
        ),
        rng
    );

    Heap heap(heap_size_rng());

    // And test its performance
    bench.run(
        "btx::memory alloc and free benchmark", [&] {

        // for(uint32_t count = 0; count < 50; ++count) {
            std::size_t alloc = 0;

            // Allocate the first half
            do {
                // ::printf("alloc %zu: %zu bytes\n", alloc, alloc_sizes[alloc]);
                allocs[alloc] = heap.alloc(alloc_sizes[alloc]);

                // Zero the memory
                ::memset(allocs[alloc], 0, alloc_sizes[alloc]);

                // Write some "useful" information
                auto *new_block = static_cast<std::size_t *>(allocs[alloc]);
                *new_block = alloc_sizes[alloc];

                ++alloc;
            } while(alloc < alloc_count/2);

            // Free the first half in random order
            for(auto const index : free_order_first_half) {
                // ::printf("  free %zu: %zu bytes\n", index, alloc_sizes[index]);
                heap.free(allocs[index]);
            }

            // Allocate the second half
            do {
                // ::printf("alloc %zu: %zu bytes\n", alloc, alloc_sizes[alloc]);
                allocs[alloc] = heap.alloc(alloc_sizes[alloc]);

                // Same nonosense as above
                ::memset(allocs[alloc], 0, alloc_sizes[alloc]);
                auto *new_block = static_cast<std::size_t *>(allocs[alloc]);
                *new_block = alloc_sizes[alloc];

                ++alloc;
            } while(alloc < alloc_count);

            // Free the second half in random order
            for(auto const index : free_order_second_half) {
                // ::printf("  free %zu: %zu bytes\n", index, alloc_sizes[index]);
                heap.free(allocs[index]);
            }
        }
    );
//*/
}