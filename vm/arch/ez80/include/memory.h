#ifndef PICOBIT_ARCH_EZ80_MEMORY_H
#define PICOBIT_ARCH_EZ80_MEMORY_H
#include <debug.h>
#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>

#include "cemu-debug.h"

#define MAX_CLEANUPS 32
struct alignas(uint24_t) CleanupHook {
    using Destructor = void(*)(void*);
    void* obj = nullptr; // not a nullptr -
    Destructor destructor = (Destructor)0x66 /* flash exception handler - will reset the calc if actually called */;
    /// idempotent cleanup invocation
    void operator()();
    static CleanupHook cleanups[MAX_CLEANUPS];
    static uint8_t activeCleanups;    
    static void cleanup();

    template<typename T>
    static void registerCleanup(T* obj) {
        if (activeCleanups >= MAX_CLEANUPS) {
            debug_printf("Couldn't register cleanup of obj at %p, likely resource leak!\n", (void*)obj);
            ENTER_DEBUGGER();
        } else {
            debug_printf("Registering cleanup[%hhu] of obj %p with pseudo-destructor %p\n",
                         activeCleanups,(void*)obj,(&T::UnsafeEvilDestroyMe));
            new (&(cleanups[activeCleanups++])) CleanupHook{ (void*)obj, (CleanupHook::Destructor)(&T::UnsafeEvilDestroyMe)};
        }
    }

    template<typename T>
    static void unregisterCleanup(T* obj) {
        if (activeCleanups == 0) {
            debug_printf("Couldn't unregister cleanup! the stack is empty?\n");
            ENTER_DEBUGGER();
        } else if (uint8_t top = activeCleanups - 1;
                   (void*)obj != cleanups[top].obj)
        {
            debug_printf("Mismatched cleanup pointers %p and %p - out of order registration/unwinding?!\n",
                         (void*)obj, (void*)(cleanups[top].obj));
            ENTER_DEBUGGER();
        } else if ((CleanupHook::Destructor)(&T::UnsafeEvilDestroyMe) != cleanups[top].destructor) {
            debug_printf("Memory corruption? Destructor addresses don't match! %p and %p!\n",
                         (void*)(&T::UnsafeEvilDestroyMe), (void*)(cleanups[top].destructor));
            ENTER_DEBUGGER();
        } else {
            debug_printf("Unregistering cleanup[%hhu] of obj %p with pseudo-destructor %p\n",
                         top,(void*)obj,(&T::UnsafeEvilDestroyMe));
            new (&(cleanups[activeCleanups = top])) CleanupHook{};
        }
    }
};

extern "C" {
    [[nodiscard, __gnu__::__malloc__]] void* bump_malloc(const size_t size);
    void bump_free(void * ptr);

    void bump_init(); /// must happen before any calls to bump_malloc or bump_free.
}

template<typename T>
class alignas(T*) BumpPointer {
    T* ptr_;
public:
    BumpPointer() : ptr_(nullptr) {}
    BumpPointer(size_t count) : ptr_(reinterpret_cast<T*>(bump_malloc(count * sizeof(T)))) {
        CleanupHook::registerCleanup(this);
    }

    BumpPointer(const BumpPointer&) = delete;
    BumpPointer(BumpPointer&& other) = delete; /*too much work making moveable cleanups for now*//*: BumpPointer() {
        // TODO - is this swap atomic / can bad things happen if, e.g. an interrupt fires?
        std::swap(ptr_, other.ptr_);
    }*/
    BumpPointer& operator=(const BumpPointer&) = delete;
    BumpPointer& operator=(BumpPointer&& other) = delete; /*too much work making moveable cleanups for now*//* {
        this->~BumpPointer(); // self destruct
        return *(new (this) BumpPointer(std::move(other))); // self reconstruct
    } */


    BumpPointer& operator=(const std::nullptr_t) {
        this->~BumpPointer(); // self destruct
        return *(new (this) BumpPointer()); // self reconstruct
    }

    ~BumpPointer() {
        debug_printf("Freeing BumpPointer %p\n", (void*)ptr_);
        void* cleanup = ptr_;
        ptr_ = nullptr;
        bump_free(cleanup);
        CleanupHook::unregisterCleanup(this);
    }

    const T* get() const {
        return ptr_;
    }

    T* get() {
        return ptr_;
    }

    T& operator[](std::size_t idx)       { return ptr_[idx]; }
    const T& operator[](std::size_t idx) const { return ptr_[idx]; }
private:
    friend struct CleanupHook;
    static void UnsafeEvilDestroyMe(BumpPointer<T> * self) {
        self->~BumpPointer();
    }
};


// TODO: PR this into the toolchain library
template<class T, class U>
struct is_same : std::false_type {};
 
template<class T>
struct is_same<T, T> : std::true_type {};
// end TODO

template<typename T, uintptr_t Ptr>
class IndirectBumpPointer {
    static void* backup;
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
            backup = *(void**)Ptr;
            debug_printf("Backing up *%p = %p\n", (void*)Ptr, backup);
            *(T**)Ptr = reinterpret_cast<T*>(bump_malloc(count * sizeof(T)));
            CleanupHook::registerCleanup(this);
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
            debug_printf("Freeing IndirectBumpPointer *%p = %p\n", (void*)Ptr, (void*)(*(T**)Ptr));
            void* cleanup = *(T**)Ptr;
            debug_printf("Restoring backup *%p = %p\n", (void*)Ptr, backup);
            *(void**)Ptr = backup;
            backup = nullptr;
            bump_free(cleanup);
            CleanupHook::unregisterCleanup(this);
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
private:
    friend struct CleanupHook;
    static void UnsafeEvilDestroyMe(IndirectBumpPointer<T, Ptr> * self) {
        self->~IndirectBumpPointer();
    }
};


template<typename T, uintptr_t Ptr> void* IndirectBumpPointer<T, Ptr>::backup = nullptr;
#ifndef NDEBUG
template<typename T, uintptr_t Ptr> size_t IndirectBumpPointer<T, Ptr>::alive = 0;
#endif

extern "C" {
#endif
#define CODE_START 0x0000

extern uint8_t ram_mem[];
#define ram_get(a) ram_mem[a]
#define ram_set(a,x) ram_mem[a] = (x)

#define ROM_BYTES 8192

extern const uint8_t *const rom_mem; // provided by linker
#define rom_get(a) (rom_mem[(a)-CODE_START])
#ifdef __cplusplus
}
#endif
#endif