#include <catch2/catch_test_macros.hpp>
#include "brasstacks/memory/BlockHeader.hpp"
#include "brasstacks/memory/Heap.hpp"

using namespace btx::memory;

TEST_CASE("Heap creation and initial state metrics") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    // First check the heap's internal metrics
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());

    // Next check the heap's structure
    char *raw_heap = heap.raw_heap();
    auto *free_header = reinterpret_cast<BlockHeader *>(raw_heap);

    REQUIRE(free_header->size == heap_size - sizeof(BlockHeader));
    REQUIRE(free_header->prev == nullptr);
    REQUIRE(free_header->next == nullptr);
}

TEST_CASE("Allocate and free a single block") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    // Allocate one block
    std::size_t const size_a = 64;
    void *alloc_a = heap.alloc(size_a);
    ::memset(alloc_a, 0, size_a);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() == size_a + 2 * sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 1);
    REQUIRE(heap.peak_used() == heap.current_used());
    REQUIRE(heap.peak_allocs() == heap.current_allocs());

    // Check that the BlockHeader helper functions produce interchangable
    // addresses
    BlockHeader *header_a = BlockHeader::header(alloc_a);
    REQUIRE(header_a->size == size_a);
    REQUIRE(alloc_a == BlockHeader::payload(header_a));

    // The header for our sole allocation is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // The free block header is now at +96 bytes
    auto *free_header = reinterpret_cast<BlockHeader *>(
        raw_heap + sizeof(BlockHeader) + header_a->size
    );

    // And the free block is 384 bytes in size
    REQUIRE(free_header->size ==
        heap_size - (
            2 * sizeof(BlockHeader)
            + header_a->size
        )
    );

    // Now free the block
    heap.free(alloc_a);

    // The heap's internal metrics should be back to their initial state
    REQUIRE(heap.current_used() == sizeof(BlockHeader));
    REQUIRE(heap.current_allocs() == 0);
    REQUIRE(heap.peak_used() == size_a + 2 * sizeof(BlockHeader));
    REQUIRE(heap.peak_allocs() == 1);
}

TEST_CASE("Allocate and free two blocks, free a->b") {
    std::size_t const heap_size = 256;
    Heap heap(heap_size);

    // Allocate two blocks
    std::size_t const size_a = 64;
    std::size_t const size_b = 96;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);

    ::memset(alloc_a, 0, size_a);
    ::memset(alloc_b, 0, size_b);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() ==
        3 * sizeof(BlockHeader)
        + size_a
        + size_b);
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

    // The first header is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // The second header is at +96 bytes
    REQUIRE(reinterpret_cast<char *>(header_b) ==
        raw_heap
        + sizeof(BlockHeader)
        + size_a
    );

    // The free block header is at +252 bytes
    auto *free_header = reinterpret_cast<BlockHeader *>(
        raw_heap
        + 2 * sizeof(BlockHeader)
        + size_a
        + size_b
    );

    // And the free block is 0 bytes in size, given a 32 byte BlockHeader
    REQUIRE(free_header->size ==
        heap_size - (
            3 * sizeof(BlockHeader)
            + size_a
            + size_b
        )
    );

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

    ::memset(alloc_a, 0, size_a);
    ::memset(alloc_b, 0, size_b);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() ==
        3 * sizeof(BlockHeader)
        + size_a
        + size_b);
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

    // The first header is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // The second header is at +96 bytes
    REQUIRE(reinterpret_cast<char *>(header_b) ==
        raw_heap
        + sizeof(BlockHeader)
        + size_a
    );

    // The free block header is at +252 bytes
    auto *free_header = reinterpret_cast<BlockHeader *>(
        raw_heap
        + 2 * sizeof(BlockHeader)
        + size_a
        + size_b
    );

    // And the free block is 0 bytes in size, given a 32 byte BlockHeader
    REQUIRE(free_header->size ==
        heap_size - (
            3 * sizeof(BlockHeader)
            + size_a
            + size_b)
    );

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

TEST_CASE("Allocate and free three blocks, free a->b->c") {
    std::size_t const heap_size = 512;
    Heap heap(heap_size);

    std::size_t const size_a = 64;
    std::size_t const size_b = 96;
    std::size_t const size_c = 128;

    void *alloc_a = heap.alloc(size_a);
    void *alloc_b = heap.alloc(size_b);
    void *alloc_c = heap.alloc(size_c);

    ::memset(alloc_a, 0, size_a);
    ::memset(alloc_b, 0, size_b);
    ::memset(alloc_c, 0, size_c);

    // Check the heap's internal metrics
    REQUIRE(heap.current_used() ==
        4 * sizeof(BlockHeader)
        + size_a
        + size_b
        + size_c
    );
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

    // The first header is at the very beginning of the heap
    char *raw_heap = heap.raw_heap();
    REQUIRE(reinterpret_cast<char *>(header_a) == raw_heap);

    // The second header is at +96 bytes
    REQUIRE(reinterpret_cast<char *>(header_b) ==
        raw_heap
        + sizeof(BlockHeader)
        + size_a
    );

    // The third header is at +252 bytes
    REQUIRE(reinterpret_cast<char *>(header_c) ==
        raw_heap
        + 2 * sizeof(BlockHeader)
        + size_a
        + size_b
    );

    // The free block header is +384 bytes
    auto *free_header = reinterpret_cast<BlockHeader *>(
        raw_heap
        + 3 * sizeof(BlockHeader)
        + size_a
        + size_b
        + size_c
    );

    // And the free block is 96 bytes, given a 32 byte BlockHeader
    REQUIRE(free_header->size
        == heap_size - (
            4 * sizeof(BlockHeader)
            + size_a
            + size_b
            + size_c
        )
    );

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_a);

    // The internal metrics will largely be the same, except with size_a fewer
    // used bytes and one fewer allocs
    REQUIRE(heap.current_used() == 352);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

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

    ::memset(alloc_a, 0, size_a);
    ::memset(alloc_b, 0, size_b);
    ::memset(alloc_c, 0, size_c);

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

    ::memset(alloc_a, 0, size_a);
    ::memset(alloc_b, 0, size_b);
    ::memset(alloc_c, 0, size_c);

    BlockHeader *header_a = BlockHeader::header(alloc_a);
    BlockHeader *header_b = BlockHeader::header(alloc_b);
    BlockHeader *header_c = BlockHeader::header(alloc_c);

    auto *free_header = reinterpret_cast<BlockHeader *>(heap.raw_heap() + 384);

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_b);

    // The 96 bytes of alloc_b will have been subtracted from the total used
    REQUIRE(heap.current_used() == 320);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

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

    ::memset(alloc_a, 0, size_a);
    ::memset(alloc_b, 0, size_b);
    ::memset(alloc_c, 0, size_c);

    BlockHeader *header_a = BlockHeader::header(alloc_a);
    BlockHeader *header_b = BlockHeader::header(alloc_b);
    BlockHeader *header_c = BlockHeader::header(alloc_c);

    auto *free_header = reinterpret_cast<BlockHeader *>(heap.raw_heap() + 384);

    //--------------------------------------------------------------------------
    // Free the first block
    heap.free(alloc_b);

    // The 96 bytes of alloc_b will have been subtracted from the total used
    REQUIRE(heap.current_used() == 320);
    REQUIRE(heap.current_allocs() == 2);
    REQUIRE(heap.peak_used() == 416);
    REQUIRE(heap.peak_allocs() == 3);

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

    // There's no more free block at the end of the heap, so header_a->next
    // points nowhere
    REQUIRE(header_a->next == nullptr);

    // And the size of header_a should be the whole available heap
    REQUIRE(header_a->size == 480);
}