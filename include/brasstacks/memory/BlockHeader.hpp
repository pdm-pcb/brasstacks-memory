#ifndef BRASSTACKS_MEMORY_BLOCKHEADER_HPP
#define BRASSTACKS_MEMORY_BLOCKHEADER_HPP

#include <cstddef>

namespace btx::memory {

struct alignas(16) BlockHeader final {
    // Convenience functions for common casting and pointer math
    [[nodiscard]] static inline BlockHeader * header(void *address) {
        return reinterpret_cast<BlockHeader *>(
            reinterpret_cast<char *>(address) - sizeof(BlockHeader)
        );
    }

    [[nodiscard]] static inline void * payload(BlockHeader *header) {
        return reinterpret_cast<char *>(header) + sizeof(BlockHeader);
    }

    // No constructors because BlockHeader is intended to be used as a means by
    // which to interpret existing memory via casts.
    BlockHeader() = delete;
    ~BlockHeader() = delete;

    BlockHeader(BlockHeader &&) = delete;
    BlockHeader(BlockHeader const &) = delete;

    BlockHeader & operator=(BlockHeader &&) = delete;
    BlockHeader & operator=(BlockHeader const &) = delete;

    std::size_t size = 0; // The size stored here refers to the space available
                          // for user allocation. Said another way, it's the
                          // size of the whole block, minus sizeof(BlockHeader).

    BlockHeader *next = nullptr;
    BlockHeader *prev = nullptr;

private:
    std::size_t const _padding = 0;
};

} // namespace btx::memory

#endif // BRASSTACKS_MEMORY_BLOCKHEADER_HPP