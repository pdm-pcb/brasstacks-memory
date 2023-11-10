#ifndef BRASSTACKS_MEMORY_BLOCKHEADER_HPP
#define BRASSTACKS_MEMORY_BLOCKHEADER_HPP

#include <cstddef>

namespace btx::memory {

struct alignas(32) BlockHeader final {
public:
    // Convenience functions for common casting and pointer math
    [[nodiscard]] static inline BlockHeader * header(void *address) {
        return reinterpret_cast<BlockHeader *>(address) - 1;
    }

    [[nodiscard]] static inline void * payload(BlockHeader *header) {
        return header + 1;
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
};

} // namespace btx::memory

#endif // BRASSTACKS_MEMORY_BLOCKHEADER_HPP