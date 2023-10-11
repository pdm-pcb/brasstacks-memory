#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;
float constexpr epsilon = 1.0e-6f;

TEST_CASE("Allocate five blocks, free a and c, then allocate a block that's "
          "less than c, testing fragmentation")
{
    std::size_t const heap_size = 1024;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;
    std::size_t const size_d = 96;
    std::size_t const size_e = 64;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);
    void *alloc_d = heap.alloc(size_d);
    void *alloc_e = heap.alloc(size_e);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == 640);
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
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);
    REQUIRE(reinterpret_cast<char *>(header_b) == raw_heap + 96);
    REQUIRE(reinterpret_cast<char *>(header_c) == raw_heap + 224);
    REQUIRE(reinterpret_cast<char *>(header_d) == raw_heap + 384);
    REQUIRE(reinterpret_cast<char *>(header_e) == raw_heap + 512);

    // And the free block is 384 bytes in size, given a 32 byte BlockHeader
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 608);
    REQUIRE(free_header->size == 384);

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_a);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 576);
    REQUIRE(heap.current_allocs() == 4);
    REQUIRE(heap.peak_used() == 640);
    REQUIRE(heap.peak_allocs() == 5);

    // 64+384=448 bytes free, so that's ~0.14 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.142857143f, epsilon));

    // header_a has become the "true" free_header, which means the next pointer
    // directs us to the free chunk at the end of the heap
    REQUIRE(header_a->next == free_header);

    // header_a, while now free, has the same size as it did before
    REQUIRE(header_a->size == 64);

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_c);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 448);
    REQUIRE(heap.current_allocs() == 3);
    REQUIRE(heap.peak_used() == 640);
    REQUIRE(heap.peak_allocs() == 5);

    // 64+128+384=576 bytes free, so that's ~0.33 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs((1.0f/3.0f), epsilon));

    // alloc_c was before the free block at the end, so now header_a->next
    // points there
    REQUIRE(header_a->next == header_c);
    REQUIRE(header_c->next == free_header);

    //--------------------------------------------------------------------------
    // Allocate a smaller chunk where alloc_c used to be
    std::size_t const size_f = 96;
    void *alloc_f = heap.alloc(size_f);
    BlockHeader *header_f = BlockHeader::header(alloc_f);
    REQUIRE(alloc_f == BlockHeader::payload(header_f));
    REQUIRE(header_f->size == size_f);

    // The newest allocation, f, will live where c once was.
    REQUIRE(alloc_f == alloc_c);
    REQUIRE(header_f == header_c);

    // Given the size request of f, header_a->next will be a block of zero,
    // but the next block will be the remainder of free space: 384 bytes

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // here, let's test that header_a->next == header_f + 32 + 96, right?

    REQUIRE(header_a->next->size == 0);
    REQUIRE(header_a->next->next->size == 384);
}


TEST_CASE("Allocate five blocks, free e and c, then allocate a block that's "
          "less than c, testing fragmentation")
{
    std::size_t const heap_size = 1024;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;
    std::size_t const size_d = 96;
    std::size_t const size_e = 64;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);
    void *alloc_d = heap.alloc(size_d);
    void *alloc_e = heap.alloc(size_e);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == 640);
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
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);
    REQUIRE(reinterpret_cast<char *>(header_b) == raw_heap + 96);
    REQUIRE(reinterpret_cast<char *>(header_c) == raw_heap + 224);
    REQUIRE(reinterpret_cast<char *>(header_d) == raw_heap + 384);
    REQUIRE(reinterpret_cast<char *>(header_e) == raw_heap + 512);

    // And the free block is 384 bytes in size, given a 32 byte BlockHeader
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 608);
    REQUIRE(free_header->size == 384);

    //--------------------------------------------------------------------------
    // Free the last block
    heap.free(alloc_e);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 544);
    REQUIRE(heap.current_allocs() == 4);
    REQUIRE(heap.peak_used() == 640);
    REQUIRE(heap.peak_allocs() == 5);

    // The last block will coallesce with the free space behind it, resulting
    // in zero fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // header_e has taken over for the original free_header, so there is no
    // next
    REQUIRE(header_e->next == nullptr);

    // The total free space has grown as well, to 480 bytes
    REQUIRE(header_e->size == 480);

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_c);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 416);
    REQUIRE(heap.current_allocs() == 3);
    REQUIRE(heap.peak_used() == 640);
    REQUIRE(heap.peak_allocs() == 5);

    // 128+480=608 bytes free, so that's ~0.2 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.2105263f, epsilon));

    // header_c->next now points to the free block at the end of the heap
    REQUIRE(header_c->next == header_e);

//     //--------------------------------------------------------------------------
//     // Allocate a smaller chunk where alloc_c used to be
//     std::size_t const size_f = 96;
//     void *alloc_f = heap.alloc(size_f);
//     BlockHeader *header_f = BlockHeader::header(alloc_f);
//     REQUIRE(alloc_f == BlockHeader::payload(header_f));
//     REQUIRE(header_f->size == size_f);

//     // The newest allocation, f, will live where c once was.
//     REQUIRE(alloc_f == alloc_c);
//     REQUIRE(header_f == header_c);

//     // Given the size request of f, header_a->next will be a block of zero,
//     // but the next block will be the remainder of free space: 384 bytes
//     REQUIRE(header_a->next->size == 0);
//     REQUIRE(header_a->next->next->size == 384);
}
