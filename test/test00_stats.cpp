#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;

TEST_CASE("Heap creation and initial state metrics") {
    std::size_t const heap_size = 512;
    Heap const heap(heap_size);

    // First check the heap's internal metrics
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));
}