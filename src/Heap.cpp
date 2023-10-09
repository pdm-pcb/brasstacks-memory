#include "brasstacks/memory/Heap.hpp"
#include "brasstacks/memory/BlockHeader.hpp"

#include <iostream>

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
    assert(bytes > 0 && "Cannot allocate zero bytes");

    if(bytes <= 0) {
        return nullptr;
    }

    // Find a free block with sufficient space available
    auto *current_block = _free_head;
    while(current_block != nullptr) {
        if(current_block->size >= bytes) {
            break;
        }
    }

    // We couldn't find a block of sufficient size, so the allocation has
    // failed and the user will need to handle it how they see fit
    if(current_block == nullptr) {
        return nullptr;
    }

    // In the likely event that the block we're allocating from is larger
    // than the requested size, we'll need to split the block and adjust the
    // free list accordingly
    if(current_block->size > bytes) {
        _split_free_block(current_block, bytes);
    }
    else {
        // Otherwise we've got a perfect fit, so all that needs doing is
        // removing the new allocation from the free list
        if(current_block->next != nullptr) {
            current_block->next->prev = current_block->prev;
        }

        if(current_block->prev != nullptr) {
            current_block->prev->next = current_block->next;
        }

        // Finally, adjust _free_head if need be
        if(current_block == _free_head) {
            _free_head = _free_head->next;
        }
    }

    // The allocation is no longer a part of the free list, so remove its
    // pointer association just for tidiness
    current_block->next = nullptr;
    current_block->prev = nullptr;

    // Update the heap's metrics
    _current_used += bytes;
    _current_allocs += 1;

    if(_current_used > _peak_used) {
        _peak_used = _current_used;
    }

    if(_current_allocs > _peak_allocs) {
        _peak_allocs = _current_allocs;
    }

    // And hand the bytes requested back to the user
    return BlockHeader::payload(current_block);
}

// =============================================================================
void Heap::free(void *address) {
    assert(address != nullptr && "Cannot free nullptr");

    if(address == nullptr) {
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
        // If the newly freed block has a lower memory address than the free
        // list's current head, the freed block becomes the new head
        block_to_free->next = _free_head;
        block_to_free->prev = nullptr;
        _free_head->prev = block_to_free;

        _free_head = block_to_free;

        _coalesce_down_from_block(_free_head);
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

        // If the previous block and the newly freed block are adjacent,
        // coalesce them
        if(reinterpret_cast<BlockHeader *>(
            reinterpret_cast<char *>(BlockHeader::payload(current_block))
            + current_block->size
        ) == block_to_free)
        {
            _coalesce_down_from_block(current_block);
        }
    }

    address = nullptr;
}

// =============================================================================
Heap::Heap(std::size_t const bytes) :
    _raw_heap       { reinterpret_cast<char *>(::malloc(bytes)) },
    _current_used   { sizeof(BlockHeader) },
    _current_allocs { 0 },
    _peak_used      { sizeof(BlockHeader) },
    _peak_allocs    { 0 }
{
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
void Heap::_split_free_block(BlockHeader *block, std::size_t const bytes) {
    // Reinterpret the space just beyond what's requested as a new free block
    auto *new_free_block = reinterpret_cast<BlockHeader *>(
        reinterpret_cast<char *>(block)
        + sizeof(BlockHeader)
        + bytes
    );

    // The heap's used size increases for each header, whether free or used
    _current_used += sizeof(BlockHeader);

    // The new block's size is set to what's left of the original block
    new_free_block->size = block->size - sizeof(BlockHeader) - bytes;

    // And the allocation we'll return is shrunk proportionately
    block->size -= new_free_block->size + sizeof(BlockHeader);

    // Fix up the linked list, removing the allocation from the free list
    new_free_block->next = block->next;
    new_free_block->prev = block->prev;

    if(new_free_block->next != nullptr) {
        new_free_block->next->prev = new_free_block;
    }

    if(new_free_block->prev != nullptr) {
        new_free_block->prev->next = new_free_block;
    }

    // Finally, adjust _free_head if need be
    if(block == _free_head) {
        _free_head = new_free_block;
    }
}

// =============================================================================
void Heap::_coalesce_down_from_block(BlockHeader *top_block) {
    BlockHeader *current_block = top_block;
    while(current_block != nullptr) {
        auto *next_header_from_offset = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<char *>(BlockHeader::payload(current_block))
            + current_block->size
        );

        if(current_block->next == nullptr) {
            break;
        }

        if(current_block->next == next_header_from_offset) {
            // Sum the size
            top_block->size += sizeof(BlockHeader) + current_block->next->size;

            // Fix the list pointers
            top_block->next = current_block->next->next;
            if(top_block->next != nullptr) {
                top_block->next->prev = top_block;
            }

            // We've coalesced, so _current_used shrinks by one BlockHeader
            _current_used -= sizeof(BlockHeader);
        }
        else {
            // We only need to advance along the linked list if we didn't
            // actually coalesce above
            current_block = current_block->next;
        }
    }
}

} // namespace btx::memory