#ifndef PICOBIT_ARCH_EZ80_MEMORY_H
#define PICOBIT_ARCH_EZ80_MEMORY_H
#ifdef __cplusplus
#include <new>

extern "C" {
    [[nodiscard, __gnu__::__malloc__]] void* bump_malloc(const size_t size);
    void bump_free(void * ptr);
}

template<typename T>
class alignas(T*) BumpPointer {
    T* ptr_;
public:
    BumpPointer() : ptr_(nullptr) {}
    BumpPointer(size_t count) : ptr_(reinterpret_cast<T*>(bump_malloc(count * sizeof(T)))) {}

    BumpPointer(const BumpPointer&) = delete;
    BumpPointer(BumpPointer&& other) : BumpPointer() {
        // TODO - is this swap atomic / can bad things happen if, e.g. an interrupt fires?
        std::swap(ptr_, other.ptr_);
    }
    BumpPointer& operator=(const BumpPointer&) = delete;
    BumpPointer& operator=(BumpPointer&& other) {
        this->~BumpPointer(); // self destruct
        return *(new (this) BumpPointer(std::move(other))); // self reconstruct
    }


    BumpPointer& operator=(const std::nullptr_t) {
        this->~BumpPointer(); // self destruct
        return *(new (this) BumpPointer()); // self reconstruct
    }

    ~BumpPointer() {
        debug_printf("Freeing BumpPointer %p\n", (void*)ptr_);
        void* cleanup = ptr_;
        ptr_ = nullptr;
        bump_free(cleanup);
    }

    const T* get() const {
        return ptr_;
    }

    T* get() {
        return ptr_;
    }

    T& operator[](std::size_t idx)       { return ptr_[idx]; }
    const T& operator[](std::size_t idx) const { return ptr_[idx]; }

};

extern "C" {
#endif
#define CODE_START 0x0000

extern uint8 ram_mem[];
#define ram_get(a) ram_mem[a]
#define ram_set(a,x) ram_mem[a] = (x)

#define ROM_BYTES 8192

#if false //def _cplusplus
// Evil hack - C code will treat this as a pointer
extern BumpPointer<uint8> rom_mem;
#else
extern /*const*/ uint8* rom_mem;
#endif
#define rom_get(a) (rom_mem[(a)-CODE_START])
#ifdef __cplusplus
}
#endif
#endif