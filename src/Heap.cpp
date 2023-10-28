#include "brasstacks/memory/Heap.hpp"
#include "brasstacks/memory/BlockHeader.hpp"

#include <cassert>
#include <cstdlib>

namespace btx::memory {

// =============================================================================
float Heap::calc_fragmentation() const {
    std::size_t total_free = 0;
    std::size_t largest_free_block_size = 0;
    BlockHeader *current_header = _free_head;

    while(current_header != nullptr) {
        if(current_header->size > largest_free_block_size) {
            largest_free_block_size = current_header->size;
        }
        total_free += current_header->size;
        current_header = current_header->next;
    }

    if(total_free == 0) {
        return 0.0f;
    }

    return 1.0f - (
        static_cast<float>(largest_free_block_size)
        / static_cast<float>(total_free)
    );
}

// =============================================================================
void * Heap::alloc(std::size_t const bytes) {
    if(bytes <= 0) {
        assert(false && "Cannot allocate zero or fewer bytes");
        return nullptr;
    }

    int32_t constexpr ALIGN = sizeof(void *);
    std::size_t const rounded_bytes = (bytes + ALIGN - 1) & -ALIGN;

    // Find a free block with sufficient space available
    auto *current_header = _free_head;
    while(current_header != nullptr) {
        // The most likely case that'll fit is the block we've found is bigger
        // than what we're asking for, so we need to split it. This implies
        // the creation of a new header for the allocated chunk
        if(current_header->size >= rounded_bytes + sizeof(BlockHeader)) {
            _split_free_block(current_header, rounded_bytes);
            break;
        }

        // Much less likely, but still possible, is finding a block that fits
        // the request exactly, in which case we just need to fix the pointers
        if(current_header->size == rounded_bytes) {
            _use_whole_free_block(current_header);
            break;
        }
        current_header = current_header->next;
    }

    // We couldn't find a block of sufficient size, so the allocation has
    // failed and the user will need to handle it how they see fit
    if(current_header == nullptr) {
        assert(false && "Failed to allocate block");
        return nullptr;
    }

    // Update the heap's metrics
    _current_used += rounded_bytes;
    _current_allocs += 1;

    if(_current_used > _peak_used) {
        _peak_used = _current_used;
    }

    if(_current_allocs > _peak_allocs) {
        _peak_allocs = _current_allocs;
    }

    // And hand the bytes requested back to the user
    return BlockHeader::payload(current_header);
}

// =============================================================================
void Heap::free(void *address) {
    assert(address != nullptr && "Cannot free nullptr");

    if(address == nullptr) {
        assert(false && "Attempting to free memory twice");
        return;
    }

    // Grab the associated header from the user's pointer
    BlockHeader *block_to_free = BlockHeader::header(address);

    // Update heap stats
    _current_used -= block_to_free->size;
    _current_allocs -= 1;

    // If the free list is empty, then this block will serve as the new head
    if(_free_head == nullptr) {
        _free_head = block_to_free;
    }
    else if(block_to_free < _free_head) {
        // If the newly freed block has a earlier memory address than the free
        // list's current head, the freed block becomes the new head
        block_to_free->next = _free_head;
        block_to_free->prev = nullptr;
        _free_head->prev = block_to_free;

        _free_head = block_to_free;
    }
    else {
        // Otherwise the newly freed block will land somewhere after the head.
        // Walk the list to find a block to insert the newly free block after,
        // and break if current_block is the last free block in the list.
        auto *current_block = _free_head;
        while(block_to_free < current_block) {
            if(current_block->next == nullptr) {
                break;
            }

            current_block = current_block->next;
        }

        // Fix the list pointers
        block_to_free->next = current_block->next;
        block_to_free->prev = current_block;

        if(block_to_free->next != nullptr) {
            block_to_free->next->prev = block_to_free;
        }

        if(block_to_free->prev != nullptr) {
            block_to_free->prev->next = block_to_free;
        }
    }

    _coalesce(block_to_free);
}

// =============================================================================
Heap::Heap(std::size_t const bytes) :
    _total_size     { bytes },
    _raw_heap       { nullptr },
    _current_used   { sizeof(BlockHeader) },
    _current_allocs { 0 },
    _peak_used      { sizeof(BlockHeader) },
    _peak_allocs    { 0 }
{
    assert(bytes > 0 && "Cannot allocate zero sized heap");

    // So long as BlockHeader's size is a power of two, this rounding to a
    // multiple math is safe
    int32_t constexpr ALIGN = sizeof(BlockHeader) * 2;
    std::size_t const rounded_bytes = (bytes + ALIGN - 1) & -ALIGN;

    _raw_heap = reinterpret_cast<uint8_t *>(::malloc(rounded_bytes));

    assert(_raw_heap != nullptr && "Heap allocation failed");

    _free_head = reinterpret_cast<BlockHeader *>(_raw_heap);

    _free_head->size = bytes - sizeof(BlockHeader);
    _free_head->next = nullptr;
    _free_head->prev = nullptr;
}

Heap::~Heap() {
    ::free(_raw_heap);
}

// =============================================================================
void Heap::_split_free_block(BlockHeader *header, std::size_t const bytes) {
    // Reinterpret the space just beyond what's requested as a new free block
    auto *new_free_header = reinterpret_cast<BlockHeader *>(
        reinterpret_cast<uint8_t *>(header)
        + sizeof(BlockHeader)
        + bytes
    );

    // The heap's used size increases for each header, whether free or used
    _current_used += sizeof(BlockHeader);

    // The new block's size is set to what's left of the original block
    new_free_header->size = header->size - sizeof(BlockHeader) - bytes;

    // And the allocation we'll return is shrunk proportionately
    header->size -= new_free_header->size + sizeof(BlockHeader);

    // Fix up the linked list, removing the allocation from the free list
    new_free_header->next = header->next;
    new_free_header->prev = header->prev;

    header->next = nullptr;
    header->prev = nullptr;

    if(new_free_header->next != nullptr) {
        new_free_header->next->prev = new_free_header;
    }

    if(new_free_header->prev != nullptr) {
        new_free_header->prev->next = new_free_header;
    }

    // Finally, adjust _free_head if need be
    if(header == _free_head) {
        _free_head = new_free_header;
    }
}

// =============================================================================
void Heap::_use_whole_free_block(BlockHeader *header) {
    if(header->next != nullptr) {
        header->next->prev = header->prev;
    }

    if(header->prev != nullptr) {
        header->prev->next = header->next;
    }

    if(header == _free_head) {
        _free_head = _free_head->next;
    }

    header->next = nullptr;
    header->prev = nullptr;
}

// =============================================================================
void Heap::_coalesce(BlockHeader *header) {

    if(header->next != nullptr) {
        // If the current block's payload plus its own size is the same
        // location as header->next, that means the blocks are contiguous
        auto *next_header_from_offset = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<uint8_t *>(BlockHeader::payload(header))
            + header->size
        );

        if(next_header_from_offset == header->next) {
            // Grow the size of the current block by absorbing the next
            auto *next_header = header->next;
            header->size += sizeof(BlockHeader) + next_header->size;

            // Fix the pointers
            header->next = next_header->next;

            if(header->next != nullptr) {
                header->next->prev = header;
            }

            next_header->next = nullptr;
            next_header->prev = nullptr;

            // Since two blocks merged, there's one less header being used
            _current_used -= sizeof(BlockHeader);
        }
    }

    if(header->prev != nullptr) {
        // This is the same strategy as above, but measuring forward from
        // header->prev
        auto *prev_header_from_offset = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<uint8_t *>(BlockHeader::payload(header->prev))
            + header->prev->size
        );

        if(prev_header_from_offset == header) {
            // Grow the size of the current block by absorbing the next
            auto *prev_header = header->prev;
            prev_header->size += sizeof(BlockHeader) + header->size;

            // Fix the pointers
            prev_header->next = header->next;

            if(header->next != nullptr) {
                header->next->prev = header->prev;
            }

            header->next = nullptr;
            header->prev = nullptr;

            // Since two blocks merged, there's one less header being used
            _current_used -= sizeof(BlockHeader);
        }
    }
}

} // namespace btx::memory