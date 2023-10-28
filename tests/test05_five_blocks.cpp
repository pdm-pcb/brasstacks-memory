#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;

TEST_CASE("Allocate five blocks, free a and c, then allocate a block that's "
          "less than c, testing fragmentation")
{
    std::size_t const heap_size = 4096;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;
    std::size_t const size_d = 256;
    std::size_t const size_e = 512;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);
    void *alloc_d = heap.alloc(size_d);
    void *alloc_e = heap.alloc(size_e);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == 1248);
    REQUIRE(heap.current_allocs() == 5);
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

    BlockHeader *header_e = BlockHeader::header(alloc_e);
    REQUIRE(header_e->size == size_e);
    REQUIRE(alloc_e == BlockHeader::payload(header_e));

    // Check the physical locations in memory
    uint8_t *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<uint8_t *>(header_a) == raw_heap);
    REQUIRE(reinterpret_cast<uint8_t *>(header_b) == raw_heap + 96);
    REQUIRE(reinterpret_cast<uint8_t *>(header_c) == raw_heap + 224);
    REQUIRE(reinterpret_cast<uint8_t *>(header_d) == raw_heap + 384);
    REQUIRE(reinterpret_cast<uint8_t *>(header_e) == raw_heap + 672);

    // And the free block is 384 bytes in size, given a 32 byte BlockHeader
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 1216);
    REQUIRE(free_header->size == 2848);

    // //--------------------------------------------------------------------------
    // // Free the first block
    // heap.free(alloc_a);

    // // The internal metrics will largely be the same, except with size_a fewer
    // // used bytes and one fewer allocs
    // REQUIRE(heap.current_used() == 576);
    // REQUIRE(heap.current_allocs() == 4);
    // REQUIRE(heap.peak_used() == 640);
    // REQUIRE(heap.peak_allocs() == 5);

    // // 64+384=448 bytes free, so that's ~0.14 fragmentation
    // REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.142857143f, epsilon));

    // // header_a has become the "true" free_header, which means the next pointer
    // // directs us to the free chunk at the end of the heap
    // REQUIRE(header_a->next == free_header);

    // // header_a, while now free, has the same size as it did before
    // REQUIRE(header_a->size == 64);

    // // header_a->next will point to free_header
    // REQUIRE(header_a->next == free_header);
    // REQUIRE(header_a->prev == nullptr);
    // REQUIRE(header_b->next == nullptr);
    // REQUIRE(header_b->prev == nullptr);
    // REQUIRE(header_c->next == nullptr);
    // REQUIRE(header_c->prev == nullptr);
    // REQUIRE(header_d->next == nullptr);
    // REQUIRE(header_d->prev == nullptr);
    // REQUIRE(header_e->next == nullptr);
    // REQUIRE(header_e->prev == nullptr);
    // REQUIRE(free_header->next == nullptr);
    // REQUIRE(free_header->prev == header_a);

    // //--------------------------------------------------------------------------
    // // Free the third block
    // heap.free(alloc_c);

    // // The internal metrics will largely be the same, except with size_a fewer
    // // used bytes and one fewer allocs
    // REQUIRE(heap.current_used() == 448);
    // REQUIRE(heap.current_allocs() == 3);
    // REQUIRE(heap.peak_used() == 640);
    // REQUIRE(heap.peak_allocs() == 5);

    // // 64+128+384=576 bytes free, so that's ~0.33 fragmentation
    // REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs((1.0f/3.0f), epsilon));

    // // alloc_c was before the free block at the end, so now header_a->next
    // // points there
    // REQUIRE(header_a->next == header_c);
    // REQUIRE(header_a->prev == nullptr);
    // REQUIRE(header_b->next == nullptr);
    // REQUIRE(header_b->prev == nullptr);
    // REQUIRE(header_c->next == free_header);
    // REQUIRE(header_c->prev == header_a);
    // REQUIRE(header_d->next == nullptr);
    // REQUIRE(header_d->prev == nullptr);
    // REQUIRE(header_e->next == nullptr);
    // REQUIRE(header_e->prev == nullptr);
    // REQUIRE(free_header->next == nullptr);
    // REQUIRE(free_header->prev == header_c);

    // //--------------------------------------------------------------------------
    // // Allocate a smaller chunk where alloc_c used to be
    // std::size_t const size_f = 96;
    // void *alloc_f = heap.alloc(size_f);
    // BlockHeader *header_f = BlockHeader::header(alloc_f);
    // REQUIRE(alloc_f == BlockHeader::payload(header_f));
    // REQUIRE(header_f->size == size_f);

    // // The newest allocation, f, will live where c once was.
    // REQUIRE(alloc_f == alloc_c);
    // REQUIRE(header_f == header_c);

    // // Given the size request of f, header_a->next will be a block of zero,
    // // but the next block will be the remainder of free space: 384 bytes

    // // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // // here, let's test that header_a->next == header_f + 32 + 96, right?

    // REQUIRE(header_a->next->size == 0);
    // REQUIRE(header_a->next->next->size == 384);
}