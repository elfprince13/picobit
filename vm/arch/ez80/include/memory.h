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


// TODO: PR this into the toolchain library
template<class T, class U>
struct is_same : std::false_type {};
 
template<class T>
struct is_same<T, T> : std::true_type {};
// end TODO

template<typename T, uintptr_t Ptr>
class IndirectBumpPointer {
#ifndef NDEBUG
    static size_t alive;
#endif
public:
    template<typename ErrCallback = std::nullptr_t>
    IndirectBumpPointer(size_t count, [[maybe_unused]] ErrCallback errCallback = ErrCallback{}) {
#ifndef NDBEUG
        alive += 1;
        if (1 == alive) {
#endif
            *(T**)Ptr = reinterpret_cast<T*>(bump_malloc(count * sizeof(T)));
#ifndef NDBEUG
        } else {
            if constexpr (!is_same<ErrCallback, std::nullptr_t>::value) {
                errCallback(alive);
            }
        }
#endif
    }

    IndirectBumpPointer(const IndirectBumpPointer&) = delete;
    IndirectBumpPointer(IndirectBumpPointer&& other) = delete;
    IndirectBumpPointer& operator=(const IndirectBumpPointer&) = delete;
    IndirectBumpPointer& operator=(IndirectBumpPointer&& other) = delete;

    ~IndirectBumpPointer() {
#ifndef NDEBUG
        alive -=1;
        if (!alive) {
#endif
            debug_printf("Freeing IndirectBumpPointer *%p = %p\n", (void*)(T**)Ptr, (void*)(*(T**)Ptr));
            void* cleanup = *(T**)Ptr;
            *(T**)Ptr = nullptr;
            bump_free(cleanup);
#ifndef NDEBUG
        }
#endif
    }

    const T* get() const {
        return *(T**)Ptr;
    }

    T* get() {
        return *(T**)Ptr;
    }

    T& operator[](std::size_t idx)       { return (*(T**)Ptr)[idx]; }
    const T& operator[](std::size_t idx) const { return (*(T**)Ptr)[idx]; }

};

#ifndef NDEBUG
template<typename T, uintptr_t Ptr> size_t IndirectBumpPointer<T, Ptr>::alive = 0;
#endif

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
extern const uint8* rom_mem;
#endif
#define rom_get(a) (rom_mem[(a)-CODE_START])
#ifdef __cplusplus
}
#endif
#endif