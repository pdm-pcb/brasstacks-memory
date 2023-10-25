#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;

TEST_CASE("Allocate and free two blocks, free a->b") {
    std::size_t const heap_size = 256;
    Heap heap(heap_size);

    // Allocate two blocks
    std::size_t const size_a = 64;
    std::size_t const size_b = 96;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == 3 * sizeof(BlockHeader) + size_a + size_b);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());

    // Check that the BlockHeader helper functions produce interchangable
    // addresses
    BlockHeader *header_a = BlockHeader::header(alloc_a);
    REQUIRE(header_a->size == size_a);
    REQUIRE(alloc_a == BlockHeader::payload(header_a));

    BlockHeader *header_b = BlockHeader::header(alloc_b);
    REQUIRE(header_b->size == size_b);
    REQUIRE(alloc_b == BlockHeader::payload(header_b));

    // Both headers' next pointer should be null
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_b->next == nullptr);

    // The first header is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // The second header is at +96 bytes
    REQUIRE(reinterpret_cast<char *>(header_b) ==
        raw_heap + sizeof(BlockHeader) + size_a
    );

    // The free block header is at +224 bytes
    auto *free_header = reinterpret_cast<BlockHeader *>(
        raw_heap + 2 * sizeof(BlockHeader) + size_a + size_b
    );

    // And the free block is 0 bytes in size, given a 32 byte BlockHeader
    REQUIRE(free_header->size ==
        heap_size - (3 * sizeof(BlockHeader) + size_a + size_b)
    );

    // free_header->next should point nowhere
    REQUIRE(free_header->next == nullptr);

    // Now free the first block
    heap.free(alloc_a);

    // Check the heap stats
    REQUIRE(heap.current_used() == size_b + 3 * sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 3 * sizeof(BlockHeader) + size_a + size_b);
    REQUIRE(heap.peak_allocs() == 2);

    // At this point, we've got a free block of 64 behind header_b and what
    // was previously called free_header is the second free block in the list
    REQUIRE(header_a->next == free_header);
    REQUIRE(free_header->prev == header_a);
    REQUIRE(header_a->size == size_a);
    REQUIRE(free_header->size == heap_size - (3 * sizeof(BlockHeader)
                                              + size_a
                                              + size_b));

    // Free the second block
    heap.free(alloc_b);

    // Check the heap stats
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == 3 * sizeof(BlockHeader) + size_a + size_b);
    REQUIRE(heap.peak_allocs() == 2);

    // Given that the second block was between the first and free blocks, the
    // entire heap should now be back to a single free block
    REQUIRE(header_a->size == heap_size - sizeof(BlockHeader));
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
}

TEST_CASE("Allocate and free two blocks, free b->a") {
    std::size_t const heap_size = 256;
    Heap heap(heap_size);

    // Allocate two blocks
    std::size_t const size_a = 64;
    std::size_t const size_b = 96;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == 3 * sizeof(BlockHeader) + size_a + size_b);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());

    // Check that the BlockHeader helper functions produce interchangable
    // addresses
    BlockHeader *header_a = BlockHeader::header(alloc_a);
    REQUIRE(header_a->size == size_a);
    REQUIRE(alloc_a == BlockHeader::payload(header_a));

    BlockHeader *header_b = BlockHeader::header(alloc_b);
    REQUIRE(header_b->size == size_b);
    REQUIRE(alloc_b == BlockHeader::payload(header_b));

    // Both headers' next pointer should be null
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_b->next == nullptr);

    // The first header is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // The second header is at +96 bytes
    REQUIRE(reinterpret_cast<char *>(header_b) ==
        raw_heap
        + sizeof(BlockHeader)
        + size_a
    );

    // The free block header is at +224 bytes
    auto *free_header = reinterpret_cast<BlockHeader *>(
        raw_heap + 2 * sizeof(BlockHeader) + size_a + size_b
    );

    // And the free block is 0 bytes in size, given a 32 byte BlockHeader
    REQUIRE(free_header->size ==
        heap_size - (3 * sizeof(BlockHeader) + size_a + size_b)
    );

    // free_header->next should point nowhere
    REQUIRE(free_header->next == nullptr);

    // Now free the second block
    heap.free(alloc_b);

    // Check the heap stats
    REQUIRE(heap.current_used() == size_a + 2 * sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 3 * sizeof(BlockHeader) + size_a + size_b);
    REQUIRE(heap.peak_allocs() == 2);

    // At this point, the free list is just header_b plus the 32 bytes it
    // absorbed when we coalesced the zero-byte block previosuly called
    // free_header
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->size == size_b + sizeof(BlockHeader));
    REQUIRE(header_a->size == size_a);

    // Free the first block
    heap.free(alloc_a);

    // Check the heap stats
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == 3 * sizeof(BlockHeader) + size_a + size_b);
    REQUIRE(heap.peak_allocs() == 2);

    // The entire heap should now be back to a single free block
    REQUIRE(header_a->size == heap_size - sizeof(BlockHeader));
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
}
