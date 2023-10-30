#ifndef BRASSTACKS_MEMORY_HEAP_HPP
#define BRASSTACKS_MEMORY_HEAP_HPP

#include "brasstacks/memory/BlockHeader.hpp"

#include <cstddef>
#include <cstdint>

namespace btx::memory {

struct BlockHeader;

class Heap final {
public:
    [[nodiscard]] void * alloc(std::size_t const bytes);
    void free(void *address);

    [[nodiscard]] std::size_t total_size()     const { return _total_size;     }
    [[nodiscard]] std::size_t current_used()   const { return _current_used;   }
    [[nodiscard]] std::size_t current_allocs() const { return _current_allocs; }
    [[nodiscard]] std::size_t peak_used()      const { return _peak_used;      }
    [[nodiscard]] std::size_t peak_allocs()    const { return _peak_allocs;    }

    [[nodiscard]] uint8_t     const * raw_heap()  const { return _raw_heap; }
    [[nodiscard]] BlockHeader const * free_head() const { return _free_head; }

    [[nodiscard]] float calc_fragmentation() const;

    Heap() = delete;
    ~Heap();

    explicit Heap(std::size_t const bytes);

    Heap(Heap &&other) = delete;
    Heap(Heap const &) = delete;

    Heap & operator=(Heap &&other) = delete;
    Heap & operator=(Heap const &) = delete;

private:
    uint8_t     *_raw_heap;
    BlockHeader *_free_head;

    std::size_t _total_size;
    std::size_t _current_used;
    std::size_t _current_allocs;
    std::size_t _peak_used;
    std::size_t _peak_allocs;

    static std::size_t constexpr _min_alloc_bytes = sizeof(BlockHeader);

    void _split_free_block(BlockHeader *header, std::size_t const bytes);
    void _use_whole_free_block(BlockHeader *header);
    void _coalesce(BlockHeader *header);
};

} // namespace btx::memory

#endif // BRASSTACKS_MEMORY_HEAP_HPP