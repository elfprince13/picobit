#include <cstdlib>
#include <picobit.h> // annoyingly include order matters - must come before memory.h
#include <arch/memory.h>

#include <ti/vars.h>

#define os_UserMem ((std::byte*)0xD1A881)
#define os_FlagsIY ((uint8_t*)0xD00080)

uint8 CleanupHook::activeCleanups = 0;
CleanupHook CleanupHook::cleanups[MAX_CLEANUPS] = {CleanupHook{}};

void CleanupHook::operator()() {
    if (void * const tmp = obj;
        obj != nullptr)
    {
        destructor(tmp); // This should handle zeroing out the data with a call to unregister.
#ifndef NDEBUG
        if(obj != nullptr || destructor != nullptr/*(Destructor)0x66*/) {
            debug_printf("Warning! post destruction cleanup hook does not appear to have invoked unregister?\n");
        }
#endif
    }
}

void CleanupHook::cleanup() {
    const uint8_t totalCleanups = activeCleanups;
    while (activeCleanups) {
        debug_printf("Cleaning up %hhu / %hhu objects!\n", activeCleanups, totalCleanups);
        const uint8_t nextCleanup = activeCleanups - 1;
        cleanups[nextCleanup]();
#ifndef NDEBUG
        if(activeCleanups != nextCleanup) {
            debug_printf("Warning! cleanup[%hhu] function %p for obj %p did not self-unregister! "
                         "This will infinite loop in release builds\n",
                         nextCleanup, cleanups[nextCleanup].destructor, cleanups[nextCleanup].obj);
            activeCleanups = nextCleanup;
        }
#endif

    }
    //ENTER_DEBUGGER();
}

static inline size_t memChk() {
    size_t bytesAvail = 0;
    //ENTER_DEBUGGER();
    asm volatile (
        "call\t0204FCh\n" // MemChk
        : "=l" (bytesAvail)
        : "iyl" (os_FlagsIY)
        : "c", "cc" // stepping through MemChk shows clobbers flags which is undocumented in 83+ WikiTI entry.
    );
    return bytesAvail;
}

extern "C" {
    uint8_t ram_mem[RAM_BYTES + VEC_BYTES] = {0};

    static uint24_t bumpFloor;
    // TODO: calc84maniac says:
    // incidentally there's an __attribute__((__tiflags__)) that can 
    // be set on function declarations for an ABI that sets IY to flags
    // you can see it used in the OS function headers in the toolchain 
    // for the functions that need it (which are C ABI other than that)
    void* bump_malloc(const size_t size) {
        debug_printf("bump_malloc: %zu bytes requested\n", size);
        
        const size_t bytesAvail = memChk();
        debug_printf("bump_malloc: %zu bytes available\n", bytesAvail);

        if (bytesAvail >= size) {
            void* result = (os_UserMem + os_AsmPrgmSize);
            debug_printf("InsertMem at %p\n", result);

            ENTER_DEBUGGER();
            asm volatile (
                "call\t020514h\n" // InsertMem
                : "=e" (result) // output - must use lower byte name for registers
                : "e"(result), "l"(size), "iyl" (os_FlagsIY) // input - must use lower byte name for registers
                : "a", "c", "cc", "memory" // clobbers - must use lower byte name for registers (except a. cc is flags) - presumably "l" should also be here but clang doesn't like that because it's an input?
            );
            debug_printf("Updating reserved os_AsmPrgmSize\n");
            os_AsmPrgmSize += size;
            return result;
        } else {
            debug_printf("Not enough memory!\n");
            std::exit(-1);//throw std::bad_alloc();
        }
    }

    void bump_free(void * ptr) {
        debug_printf("bump_free called on %p\n", ptr);
        const std::byte * curCeiling = (os_UserMem + os_AsmPrgmSize);
        if (ptr < curCeiling) {
            if (ptr >= (os_UserMem + bumpFloor)) {
                const size_t size = curCeiling - reinterpret_cast<std::byte*>(ptr);
                debug_printf("Attempting to release %zu = %p - %p bytes\n", size, curCeiling, ptr);
#ifndef NDEBUG
                debug_printf("Memory free before: %zu\n", memChk());
#endif
                //
                //memset(ptr, 0x00, size);
                //size_t actualDel;
                ENTER_DEBUGGER();
                asm volatile (
                    "call\t020590h\n" // DelMem
                    : //"=e"(size), "=c"(actualDel)
                    : "l"(ptr), "e"(size), "iyl" (os_FlagsIY)
                    : "a", "c", "cc", "memory"
                );
                os_AsmPrgmSize -= size;
#ifndef NDEBUG
                debug_printf("Memory free after: %zu\n", memChk());
#endif
                //debug_printf("DelMem released %zu / %zu bytes\n", actualDel, size);
                //assert(size == actualDel);
            } else {
                std::exit(-1);//throw bad_free("bump_free: can't free below the bump floor");
            }
        } else {
            std::exit(-1);//throw bad_free("bump_free: can't free above the bump floor (may indicate double free?)");
        }
    }

    void bump_init() {
        bumpFloor = os_AsmPrgmSize; // must happen before any calls to bump_free / bump_malloc;
        debug_printf("bump floor: %zu\n", bumpFloor);
    }
}