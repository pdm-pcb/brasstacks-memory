#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;

TEST_CASE("Allocate four blocks, free a and c, then allocate a block that's "
          "less than c, testing fragmentation")
{
    std::size_t const heap_size = 1280;
    Heap heap(heap_size);

    std::size_t const size_a = 96;
    std::size_t const size_b = 128;
    std::size_t const size_c = 256;
    std::size_t const size_d = 512;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);
    void *alloc_d = heap.alloc(size_d);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == 1152);
    REQUIRE(heap.current_allocs() == 4);
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

    BlockHeader *header_c = BlockHeader::header(alloc_c);
    REQUIRE(header_c->size == size_c);
    REQUIRE(alloc_c == BlockHeader::payload(header_c));

    BlockHeader *header_d = BlockHeader::header(alloc_d);
    REQUIRE(header_d->size == size_d);
    REQUIRE(alloc_d == BlockHeader::payload(header_d));

    // Check the physical locations in memory
    auto const *raw_heap = reinterpret_cast<std::uint8_t const *>(header_a);
    REQUIRE(reinterpret_cast<std::uint8_t *>(header_a) == raw_heap);
    REQUIRE(reinterpret_cast<std::uint8_t *>(header_b) == raw_heap + 128);
    REQUIRE(reinterpret_cast<std::uint8_t *>(header_c) == raw_heap + 288);
    REQUIRE(reinterpret_cast<std::uint8_t *>(header_d) == raw_heap + 576);

    // And the free block is 32 bytes in size, given a 32 byte BlockHeader
    auto const *free_header =
        reinterpret_cast<BlockHeader const *>(raw_heap + 1120);
    REQUIRE(free_header->size == 128);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    //--------------------------------------------------------------------------
    // Free alloc_a
    heap.free(alloc_a);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 1056);
    REQUIRE(heap.current_allocs() == 3);
    REQUIRE(heap.peak_used() == 1152);
    REQUIRE(heap.peak_allocs() == 4);

    // 96+128=224 bytes free, so that's ~0.43 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.42857143f, epsilon));

    // header_a, while now free, has the same size as it did before
    REQUIRE(header_a->size == 96);

    // As does free_header
    REQUIRE(free_header->size == 128);

    // header_a has become the "true" free_header, which means the next pointer
    // directs us to the free chunk at the end of the heap
    REQUIRE(header_a->next == free_header);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(header_d->next == nullptr);
    REQUIRE(header_d->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_a);

    //--------------------------------------------------------------------------
    // Free alloc_c
    heap.free(alloc_c);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 800);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 1152);
    REQUIRE(heap.peak_allocs() == 4);

    // 96+256+128=480 bytes free, so that's ~0.467 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.46666667f, epsilon));

    // alloc_c was before the free block at the end, so header_a->next now
    // points to alloc_c
    REQUIRE(header_a->next == header_c);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == free_header);
    REQUIRE(header_c->prev == header_a);
    REQUIRE(header_d->next == nullptr);
    REQUIRE(header_d->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_c);

    //--------------------------------------------------------------------------
    // Allocate a smaller chunk where alloc_d used to be, but larger than
    // alloc_a
    std::size_t const size_e = 128;
    void *alloc_e = heap.alloc(size_e);
    BlockHeader *header_e = BlockHeader::header(alloc_e);
    REQUIRE(alloc_e == BlockHeader::payload(header_e));
    REQUIRE(header_e->size == size_e);

    // The newest allocation, f, will live where d once was.
    REQUIRE(alloc_e == alloc_c);
    REQUIRE(header_e == header_c);

    auto *free_half_of_c =  reinterpret_cast<BlockHeader *>(
        reinterpret_cast<std::uint8_t *>(header_e)
        + sizeof(BlockHeader)
        + size_e
    );

    // Now we can test the pointer layout
    REQUIRE(header_a->next == free_half_of_c);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_e->next == nullptr);
    REQUIRE(header_e->prev == nullptr);
    REQUIRE(free_half_of_c->next == free_header);
    REQUIRE(free_half_of_c->prev == header_a);
    REQUIRE(header_d->next == nullptr);
    REQUIRE(header_d->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == free_half_of_c);

    // And the size of the new free half of C
    REQUIRE(free_half_of_c->size == 96);
}