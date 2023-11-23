#ifndef BRASSTACKS_MEMORY_HEAP_HPP
#define BRASSTACKS_MEMORY_HEAP_HPP

#include "brasstacks/memory/BlockHeader.hpp"

#include <cstdint>

// This allocator is designed for use on systems where pointers are powers of
// two in size.
#include <bit>
static_assert(std::has_single_bit(sizeof(void *)));

namespace btx::memory {

class Heap final {
public:
    [[nodiscard]] void * alloc(std::size_t const req_bytes);
    void free(void *address);

    [[nodiscard]] auto total_size()     const { return _total_size;     }
    [[nodiscard]] auto current_used()   const { return _current_used;   }
    [[nodiscard]] auto current_allocs() const { return _current_allocs; }
    [[nodiscard]] auto peak_used()      const { return _peak_used;      }
    [[nodiscard]] auto peak_allocs()    const { return _peak_allocs;    }

    [[nodiscard]] float calc_fragmentation() const;

    Heap() = delete;
    ~Heap();

    explicit Heap(std::size_t const req_bytes);

    Heap(Heap &&other) = delete;
    Heap(Heap const &) = delete;

    Heap & operator=(Heap &&other) = delete;
    Heap & operator=(Heap const &) = delete;

private:
    std::uint8_t *_raw_heap;
    BlockHeader  *_free_head;

    std::size_t const _total_size;
    std::size_t _current_used;
    std::size_t _current_allocs;
    std::size_t _peak_used;
    std::size_t _peak_allocs;

    static std::size_t constexpr _min_alloc_bytes = sizeof(BlockHeader);

    static std::size_t _round_bytes(std::size_t const req_bytes,
                                    std::size_t const multiple);

    void _split_free_block(BlockHeader *header, std::size_t const bytes);
    void _use_whole_free_block(BlockHeader *header);
    void _coalesce(BlockHeader *header);
};

} // namespace btx::memory

#endif // BRASSTACKS_MEMORY_HEAP_HPP