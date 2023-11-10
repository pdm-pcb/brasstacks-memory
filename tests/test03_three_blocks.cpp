#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

#include "test_helpers.hpp"

using namespace btx::memory;
using namespace Catch::Matchers;

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
    auto *raw_heap = reinterpret_cast<std::uint8_t *>(header_a);
    REQUIRE(reinterpret_cast<std::uint8_t *>(header_b) == raw_heap + 96);
    REQUIRE(reinterpret_cast<std::uint8_t *>(header_c) == raw_heap + 224);

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
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_a);

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
    // since it just absorbed alloc_b
    REQUIRE(header_a->next == free_header);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_a);

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
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}

TEST_CASE("Allocate and free three blocks, free a->c->b") {
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

    auto *raw_heap = reinterpret_cast<std::uint8_t *>(header_a);
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 384);

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
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_a);

    // header_a, while now free, has the same size as it did before
    REQUIRE(header_a->size == 64);

    //--------------------------------------------------------------------------
    // Free the second block
    heap.free(alloc_c);

    // header_c and free_header have merged
    REQUIRE(heap.current_used() == 192);
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // header_a is still the top of the free list, but now it points to b
    REQUIRE(header_a->next == header_c);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == header_a);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    // 64+256=320 bytes total free, fragmentation is ~0.2
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.2f, epsilon));

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_b);

    // Finally, everything's free so only the 32 bytes of the heap's header are
    // used
    REQUIRE(heap.current_used() == 32);
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // No fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.0f, epsilon));

    // There's no more free block at the end of the heap, so header_a->next
    // points nowhere
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

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
    BlockHeader *header_c = BlockHeader::header(alloc_c);

    auto *raw_heap = reinterpret_cast<std::uint8_t *>(header_a);
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 384);

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
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == free_header);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_b);

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

    // a and b taken together gives us 192 bytes, so ~0.3 fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs((1.0f/3.0f), epsilon));

    // header_a->next now jumps to the original free_header
    REQUIRE(header_a->next == free_header);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_a);

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
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

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
    BlockHeader *header_c = BlockHeader::header(alloc_c);

    auto *raw_heap = reinterpret_cast<std::uint8_t *>(header_a);
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 384);

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

    // With header_b now technically the free header, its next pointer will
    // lead to the original free_header
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == free_header);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == header_b);

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

    // All the free space is coallesced, so header_b is the whole free list
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

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

    // Everything's free
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}

TEST_CASE("Allocate and free three blocks, free c->a->b") {
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

    auto *raw_heap = reinterpret_cast<std::uint8_t *>(header_a);
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 384);

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

    // Since the original free block and alloc_c have merged, and header_c
    // is the new free header, the pointers are cleared out
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    // The header_c/the free header's size has grown
    REQUIRE(header_c->size == 256);

    //--------------------------------------------------------------------------
    // Free the second block
    heap.free(alloc_a);

    // header_a is the new free header, so we've only reclaimed 64 bytes
    REQUIRE(heap.current_used() == 192);
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

    // The free list pointers skip over alloc_b
    REQUIRE(header_a->next == header_c);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == header_a);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    // 64+256=320, and 256/320 = 0.8, so we've got ~20% fragmentation
    REQUIRE_THAT(heap.calc_fragmentation(), WithinAbs(0.2f, epsilon));

    //--------------------------------------------------------------------------
    // Free the third block
    heap.free(alloc_b);

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
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

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

    auto *raw_heap = reinterpret_cast<std::uint8_t *>(header_a);
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap + 384);

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

    // Since the original free block and alloc_c have merged, and header_c
    // is the new free header, the pointers are cleared out
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    // The header_c/the free header's size has grown
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
    REQUIRE(header_a->next == nullptr);
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

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
    REQUIRE(header_a->prev == nullptr);
    REQUIRE(header_b->next == nullptr);
    REQUIRE(header_b->prev == nullptr);
    REQUIRE(header_c->next == nullptr);
    REQUIRE(header_c->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
    REQUIRE(free_header->prev == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}