#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;
float constexpr epsilon = 1.0e-6f;

TEST_CASE("Allocate and free three blocks, free a->b->c") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == 416);
    REQUIRE(heap.current_allocs() == 3);
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

    // Check the physical locations in memory
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);
    REQUIRE(reinterpret_cast<char *>(header_b) == raw_heap + 96);
    REQUIRE(reinterpret_cast<char *>(header_c) == raw_heap + 224);

    // And the free block is 96 bytes in size, given a 32 byte BlockHeader
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 384);
    REQUIRE(free_header->size == 96);

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_a);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 352);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // Given 64+96=160 bytes total free, fragmentation is ~0.4
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.4f, epsilon));

    // header_a has become the "true" free_header, which means the next pointer
    // directs us to the free chunk at the end of the heap
    REQUIRE(header_a->next == free_header);

    // header_a, while now free, has the same size as it did before
    REQUIRE(header_a->size == 64);

    //--------------------------------------------------------------------------
    // Free the second block
    heap.free(alloc_b);

    // This time, the used bytes count drops by size_b and the size of a
    // BlockHeader, since a and b should be merged now
    REQUIRE(heap.current_used() == 224);
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // Given 192+96=288 bytes total free, fragmentation is ~0.33
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs((1.0f/3.0f), epsilon));

    // header_a->next still  points to the free block at the end of the heap
    REQUIRE(header_a->next == free_header);

    // But the size has grown by size_b and sizeof(BlockHeader)
    REQUIRE(header_a->size == 192);

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_c);

    // Finally, everything's free so only the 32 bytes of the heap's header are
    // used
    REQUIRE(heap.current_used() == 32);
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // Everything is free, so fragmentation should be at zero
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // There's no more free block at the end of the heap, so header_a->next
    // points nowhere
    REQUIRE(header_a->next == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}

TEST_CASE("Allocate and free three blocks, free c->b->a") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);

    BlockHeader *header_a = BlockHeader::header(alloc_a);
    BlockHeader *header_b = BlockHeader::header(alloc_b);
    BlockHeader *header_c = BlockHeader::header(alloc_c);

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_c);

    // The free block at the end of the heap and alloc_c will have merged
    REQUIRE(heap.current_used() == 256);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // Given the free blocks are coallesced, fragmentation is 0
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // Since the original free block and alloc_c have merged, header_c->next
    // will point nowhere
    REQUIRE(header_c->next == nullptr);

    // header_c, serving as the "new" free_header, will have grown in size
    REQUIRE(header_c->size == 256);

    //--------------------------------------------------------------------------
    // Free the second block
    heap.free(alloc_b);

    // Again, the used bytes count decreases by sizeof(BlockHeader) and alloc_b
    // due to the coallescing of free space
    REQUIRE(heap.current_used() == 128);
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // Still zero fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // Now header_b is the "new" free_header
    REQUIRE(header_b->next == nullptr);

    // header_c, serving as the "new" free_header, will have grown in size
    REQUIRE(header_b->size == 384);

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_a);

    // Finally, everything's free so only the 32 bytes of the heap's header are
    // used
    REQUIRE(heap.current_used() == 32);
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // And certainly zero fragmentation with everything free
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // There's no more free block at the end of the heap, so header_a->next
    // points nowhere
    REQUIRE(header_a->next == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}

TEST_CASE("Allocate and free three blocks, free b->a->c") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);

    BlockHeader *header_a = BlockHeader::header(alloc_a);
    BlockHeader *header_b = BlockHeader::header(alloc_b);

    auto *free_header = reinterpret_cast<BlockHeader *>(heap.raw_heap() + 384);

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_b);

    // The 96 bytes of alloc_b will have been subtracted from the total used
    REQUIRE(heap.current_used() == 320);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // alloc_b is now free and is 96 bytes, so we're at ~0.5 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.5f, epsilon));

    // With header_b now technically a free header, its next pointer will
    // lead to the original free_header
    REQUIRE(header_b->next == free_header);

    // Both header_b and free_header will have the same sizes as before
    REQUIRE(header_b->size == 96);
    REQUIRE(free_header->size == 96);

    //--------------------------------------------------------------------------
    // Free the second block
    heap.free(alloc_a);

    // Now we've got a and b merged, plus the straggler free block at the end
    REQUIRE(heap.current_used() == 224);
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // a and be taken together gives us 192 bytes, so ~0.3 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs((1.0f/3.0f), epsilon));

    // header_a->next now jumps to the original free_header
    REQUIRE(header_a->next == free_header);

    // header_a->size has grown to encompass both a and b, but free_header
    // stays the same
    REQUIRE(header_a->size == 192);
    REQUIRE(free_header->size == 96);

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_c);

    // Finally, everything's free so only the 32 bytes of the heap's header are
    // used
    REQUIRE(heap.current_used() == 32);
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // And 0 fragmentation when it's all said and done
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // There's no more free block at the end of the heap, so header_a->next
    // points nowhere
    REQUIRE(header_a->next == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}

TEST_CASE("Allocate and free three blocks, free b->c->a") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);

    BlockHeader *header_a = BlockHeader::header(alloc_a);
    BlockHeader *header_b = BlockHeader::header(alloc_b);

    auto *free_header = reinterpret_cast<BlockHeader *>(heap.raw_heap() + 384);

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_b);

    // The 96 bytes of alloc_b will have been subtracted from the total used
    REQUIRE(heap.current_used() == 320);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // alloc_b is now free and is 96 bytes, so we're at ~0.5 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.5f, epsilon));

    // With header_b now technically a free header, its next pointer will
    // lead to the original free_header
    REQUIRE(header_b->next == free_header);

    // Both header_b and free_header will have the same sizes as before
    REQUIRE(header_b->size == 96);
    REQUIRE(free_header->size == 96);

    //--------------------------------------------------------------------------
    // Free the second block
    heap.free(alloc_c);

    // Now we've just got a and b, with all the free space after b coallesced
    REQUIRE(heap.current_used() == 128);
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // b and c will have merged with the original free block, so there's no
    // fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // All the free space is already coallesced, so header_b->next is nullptr
    REQUIRE(header_b->next == nullptr);

    // header_b->size has grown to encompass c and the original free_header
    REQUIRE(header_b->size == 384);

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_a);

    // Finally, everything's free so only the 32 bytes of the heap's header are
    // used
    REQUIRE(heap.current_used() == 32);
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // And again, no fragmentation when everything's free
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // There's no more free block at the end of the heap, so header_a->next
    // points nowhere
    REQUIRE(header_a->next == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}