#ifndef BRASSTACKS_MEMORY_HEAP_HPP
#define BRASSTACKS_MEMORY_HEAP_HPP

#include "brasstacks/memory/pch.hpp"

namespace btx::memory {

struct BlockHeader;

class Heap final {
public:
    [[nodiscard]] void * alloc(std::size_t const bytes);
    void free(void *address);

    [[nodiscard]] std::size_t current_used()   const { return _current_used;   }
    [[nodiscard]] std::size_t current_allocs() const { return _current_allocs; }
    [[nodiscard]] std::size_t peak_used()      const { return _peak_used;      }
    [[nodiscard]] std::size_t peak_allocs()    const { return _peak_allocs;    }

    [[nodiscard]] char * raw_heap() { return _raw_heap; }

    Heap() = delete;
    ~Heap();

    explicit Heap(std::size_t const bytes);

    Heap(Heap &&other) = delete;
    Heap(Heap const &) = delete;

    Heap & operator=(Heap &&other) = delete;
    Heap & operator=(Heap const &) = delete;

private:
    char        *_raw_heap;
    BlockHeader *_free_head;

    std::size_t _current_used;
    std::size_t _current_allocs;
    std::size_t _peak_used;
    std::size_t _peak_allocs;

    void _split_free_block(BlockHeader *block, std::size_t const bytes);
    void _coalesce_down_from_block(BlockHeader *top_block);
};

} // namespace btx::memory

#endif // BRASSTACKS_MEMORY_HEAP_HPP