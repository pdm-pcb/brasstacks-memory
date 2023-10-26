#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;

TEST_CASE("Allocate and free a single block") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    // Allocate one block
    std::size_t const size_a = 64;
    void *alloc_a = heap.alloc(size_a);

    // Check the heap's internal metrics
    REQUIRE(heap.total_size() == 512);
    REQUIRE(heap.current_used() == size_a + 2 * sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // Check that the BlockHeader helper functions produce interchangable
    // addresses, and that header_a->next is nullptr
    BlockHeader *header_a = BlockHeader::header(alloc_a);
    REQUIRE(header_a->size == size_a);
    REQUIRE(alloc_a == BlockHeader::payload(header_a));
    REQUIRE(header_a->next == nullptr);

    // The header for our sole allocation is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // The free block header is now at +96 bytes
    auto *free_header = reinterpret_cast<BlockHeader *>(
        raw_heap + sizeof(BlockHeader) + header_a->size
    );

    // And the free block is 384 bytes in size
    REQUIRE(free_header->size ==
        heap_size - (header_a->size + 2 * sizeof(BlockHeader))
    );

    // free_header->next should point nowhere
    REQUIRE(free_header->next == nullptr);

    // Now free the block
    heap.free(alloc_a);

    // The heap's internal metrics should be back to their initial state
    REQUIRE(heap.total_size() == 512);
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == size_a + 2 * sizeof(BlockHeader));
    REQUIRE(heap.peak_allocs() == 1);
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));
}

TEST_CASE("Allocate and free a single block, filling the heap") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    // Allocate one block
    std::size_t const size_a = heap_size - sizeof(BlockHeader);
    void *alloc_a = heap.alloc(size_a);

    // Check the heap's internal metrics
    REQUIRE(heap.total_size() == 512);
    REQUIRE(heap.current_used() == size_a + sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // Check that the BlockHeader helper functions produce interchangable
    // addresses, and that header_a->next is nullptr
    BlockHeader *header_a = BlockHeader::header(alloc_a);
    REQUIRE(header_a->size == size_a);
    REQUIRE(alloc_a == BlockHeader::payload(header_a));
    REQUIRE(header_a->next == nullptr);

    // The header for our sole allocation is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // Now free the block
    heap.free(alloc_a);

    // The heap's internal metrics should be back to their initial state
    REQUIRE(heap.total_size() == 512);
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == size_a + sizeof(BlockHeader));
    REQUIRE(heap.peak_allocs() == 1);
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));
}