#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;
float constexpr epsilon = 1.0e-6f;

TEST_CASE("Heap creation and initial state metrics") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    // First check the heap's internal metrics
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // Next check the heap's structure
    char *raw_heap = heap.raw_heap();
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap);

    REQUIRE(free_header->size == heap_size - sizeof(BlockHeader));
    REQUIRE(free_header->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
}