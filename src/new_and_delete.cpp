#include "brasstacks/memory/new_and_delete.hpp"

#include <cassert>
#include <cstdio>

size_t total_bytes = 0;
size_t alloc_count = 0;
size_t free_count  = 0;

char const *success_string =
    "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "                         Mission accomplished"
    "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

// =============================================================================
void * operator new(size_t bytes) {
    total_bytes += bytes;
    alloc_count++;

    // How this works is by allocating sizeof(size_t) extra bytes on top of
    // what was requested by the user, then storing the actual requested size
    // in that extra space before returning a pointer to the rest of the space
    void *new_alloc = ::malloc(bytes + sizeof(size_t));
    static_cast<size_t *>(new_alloc)[0] = bytes;
    return &(static_cast<size_t *>(new_alloc)[1]);
}

// =============================================================================
void * operator new[](size_t bytes) {
    total_bytes += bytes;
    alloc_count++;

    void *new_alloc = ::malloc(bytes + sizeof(size_t));
    static_cast<size_t *>(new_alloc)[0] = bytes;
    return &(static_cast<size_t *>(new_alloc)[1]);
}

// =============================================================================
void * operator new(size_t bytes,
                    [[maybe_unused]] const std::nothrow_t &nothrow) noexcept
{
    total_bytes += bytes;
    alloc_count++;

    void *new_alloc = ::malloc(bytes + sizeof(size_t));
    static_cast<size_t *>(new_alloc)[0] = bytes;
    return &(static_cast<size_t *>(new_alloc)[1]);
}

// =============================================================================
void operator delete(void *memory) noexcept {
    if(memory == nullptr) {
        return;
    }

    // When the user calls delete, we retrieve the original size requested via
    // the tricky -1 index. I didn't come up with this, but I'm struggling to
    // find the StackOverflow post to give proper credit. Either way, the
    // imaginary-array-of-size_t approach is perfect for my needs.
    const size_t bytes = static_cast<size_t *>(memory)[-1];
    total_bytes -= bytes;
    free_count++;

    ::free(&(static_cast<size_t *>(memory)[-1]));
    if(total_bytes == 0) {
        ::printf("%s%zu / %zu\n",
                 success_string, alloc_count, free_count);
    }
}

// =============================================================================
void operator delete[](void *memory) noexcept {
    if(memory == nullptr) {
        return;
    }

    const size_t bytes = static_cast<size_t *>(memory)[-1];
    total_bytes -= bytes;
    free_count++;

    ::free(&(static_cast<size_t *>(memory)[-1]));
    if(total_bytes == 0) {
        ::printf("%s%zu / %zu\n",
                 success_string, alloc_count, free_count);
    }
}

// =============================================================================
void operator delete(void *memory, size_t bytes) noexcept {
    if(memory == nullptr) {
        return;
    }

    const size_t expected_bytes = static_cast<size_t *>(memory)[-1];
    assert(expected_bytes == bytes);
    total_bytes -= bytes;
    free_count++;

    ::free(&(static_cast<size_t *>(memory)[-1]));
    if(total_bytes == 0) {
        ::printf("%s%zu / %zu\n",
                 success_string, alloc_count, free_count);
    }
}

// =============================================================================
void operator delete[](void *memory, size_t bytes) noexcept {
    if(memory == nullptr) {
        return;
    }

    const size_t expected_bytes = static_cast<size_t *>(memory)[-1];
    assert(expected_bytes == bytes);
    total_bytes -= bytes;
    free_count++;

    ::free(&(static_cast<size_t *>(memory)[-1]));
    if(total_bytes == 0) {
        ::printf("%s%zu / %zu\n",
                 success_string, alloc_count, free_count);
    }
}


// =============================================================================
void operator delete(void *memory,
                     [[maybe_unused]] const std::nothrow_t &nothrow) noexcept
{
    if(memory == nullptr) {
        return;
    }

    const size_t bytes = static_cast<size_t *>(memory)[-1];
    total_bytes -= bytes;
    free_count++;

    ::free(&(static_cast<size_t *>(memory)[-1]));
    if(total_bytes == 0) {
        ::printf("%s%zu / %zu\n",
                 success_string, alloc_count, free_count);
    }
}