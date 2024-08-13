#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#include <picobit.h>
#include <dispatch.h>
#include <gc.h>

#include <ti/error.h>
#include <fileioc.h>
#include <debug.h>

#define os_UserMem ((std::byte*)0xD1A881)
#define os_FlagsIY ((uint8*)0xD00080)

#ifndef NDEBUG
#define ENTER_DEBUGGER() asm volatile (\
        "scf\n"\
        "sbc\thl,hl\n"\
        "ld     (hl),2"\
        :\
        :\
        : "l"\
    )

#define SET_FUNC_BREAKPOINT(func) asm volatile (\
        "scf\n"\
        "sbc\thl,hl\n"\
        "ld     (hl),3"\
        :\
        : "e"(&func)\
        : "l"\
    )

#define SET_LABEL_BREAKPOINT(label) asm volatile (\
        "scf\n"\
        "sbc\thl,hl\n"\
        "ld     (hl),3"\
        :\
        : "e"(&&label)\
        : "l"\
    )
#else 
#define ENTER_DEBUGGER()
#define SET_FUNC_BREAKPOINT(func)
#define SET_LABEL_BREAKPOINT(label)
#endif

/*
class bad_free : public std::exception {
    const char* what_;
public:
    bad_free(const char* what) : what_(what) {}
    const char* what() const noexcept override {
        return what_;
    }
};

class file_error : public std::exception {
    const char* what_;
public:
    file_error(const char* what) : what_(what) {}
    const char* what() const noexcept override {
        return what_;
    }
};

class scheme_error : public std::exception {
    const char* prim_;
    const char* msg_;
public:
    scheme_error(const char* prim, const char* msg) : prim_(prim), msg_(msg) {}
    const char* what() const noexcept override {
        return "Error from the scheme runtime. Please call .prim() and .msg() if you see this.";
    }

    const char* prim() const {
        return prim_;
    }

    const char* msg() const {
        return msg_;
    }
};

class scheme_type_error : public std::exception {
    const char* prim_;
    const char* type_;
public:
    scheme_type_error(const char* prim, const char* type) : prim_(prim), type_(type) {}
    const char* what() const noexcept override {
        return "Error from the scheme runtime. Please call .prim() and .type() if you see this.";
    }

    const char* prim() const {
        return prim_;
    }

    const char* type() const {
        return type_;
    }
};
*/

class TIFile {
    uint8_t handle_;
public:
    // just support app vars for now..
    TIFile(const char* name, const char* mode)
        : handle_(ti_Open(name, mode))
    {
        CleanupHook::registerCleanup(this);
    }

    TIFile(const TIFile&) = delete;
    TIFile(TIFile&& other) = delete; /*too much work making moveable cleanups for now */ /*: handle_(0) {
        // TODO - is this swap atomic / can bad things happen if, e.g. an interrupt fires?
        std::swap(handle_, other.handle_);
    } */
    TIFile& operator=(const TIFile&) = delete;
    TIFile& operator=(TIFile&& other) = delete; /*too much work making moveable cleanups for now */ /*{
        this->~TIFile(); // self destruct
        return *(new (this) TIFile(std::move(other)));
    } */

    ~TIFile() {
        debug_printf("Destroying TIFile %hhu\n", handle_);
        if(0 != handle_) {
            ti_Close(handle_);
        }
        CleanupHook::unregisterCleanup(this);
    }

    /// never 0
    operator uint8_t () const {
        return handle_;
    }
private:
    friend struct CleanupHook;
    static void UnsafeEvilDestroyMe(TIFile * self) {
        self->~TIFile();
    }
};


extern "C" {
    uint8 ram_mem[RAM_BYTES + VEC_BYTES] = {0};
    //BumpPointer<uint8> rom_mem = NULL;
    //uint8* rom_mem = NULL;

    static uint24_t bumpFloor;
    // TODO: calc84maniac says:
    // incidentally there's an __attribute__((__tiflags__)) that can 
    // be set on function declarations for an ABI that sets IY to flags
    // you can see it used in the OS function headers in the toolchain 
    // for the functions that need it (which are C ABI other than that)
    void* bump_malloc(const size_t size) {
        debug_printf("bump_malloc: %zu bytes requested\n", size);
        
        size_t bytesAvail = 0;
        asm volatile (
            "call\t0204FCh\n" // MemChk
            : "=l" (bytesAvail)
            : "iyl" (os_FlagsIY)
            : "c"
        );

        debug_printf("bump_malloc: %zu bytes available\n", bytesAvail);

        if (bytesAvail >= size) {
            void* result = (os_UserMem + os_AsmPrgmSize);
            debug_printf("InsertMem at %p\n", result);
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
                //size_t actualDel;
                os_AsmPrgmSize -= size;
                asm volatile (
                    "call\t020580h\n" // DelMemA - DelMem appears to just be a redirect?
                    : //"=e"(size), "=c"(actualDel)
                    : "l"(ptr), "e"(size), "iyl" (os_FlagsIY)
                    : "a", "c", "cc", "memory"
                );
                //debug_printf("DelMem released %zu / %zu bytes\n", actualDel, size);
                //assert(size == actualDel);
            } else {
                std::exit(-1);//throw bad_free("bump_free: can't free below the bump floor");
            }
        } else {
            std::exit(-1);//throw bad_free("bump_free: can't free above the bump floor (may indicate double free?)");
        }
    }

    void error (const char *prim, const char *msg)
    {
        snprintf(os_AppErr1, 12, "%s:%s", prim, msg); 
        std::exit(-1);//throw scheme_error(prim, msg);
        /*
        // maximum of 12 bytes, must be nul-terminated
        snprintf(os_AppErr1, 12, "%s:%s", prim, msg); 
        os_ThrowError(OS_E_APPERR1);
        */
    }

    void type_error (const char *prim, const char *type)
    {
        snprintf(os_AppErr2, 12, "%s-%s", prim, type);
        std::exit(-1);//throw scheme_type_error(prim, type);
        /*
        // maximum of 12 bytes, must be nul-terminated
        snprintf(os_AppErr2, 12, "%s-%s", prim, type);
        os_ThrowError(OS_E_APPERR2);
        */
    }
}

uint8 CleanupHook::activeCleanups = 0;
CleanupHook CleanupHook::cleanups[MAX_CLEANUPS];

void CleanupHook::operator()() {
    if (void * const tmp = obj;
        obj != nullptr)
    {
        obj = nullptr;
        destructor(tmp);
        destructor = (void(*)(void*))0x66;
    }
}

void CleanupHook::cleanup() {
    const uint8_t totalCleanups = activeCleanups;
    while (activeCleanups) {
        debug_printf("Cleaning up %hhu / %hhu objects!\n", activeCleanups, totalCleanups);
        cleanups[--activeCleanups]();
    }
}

int main ()
{
    std::atexit(&CleanupHook::cleanup);
    std::at_quick_exit(&CleanupHook::cleanup);
    ENTER_DEBUGGER();
    bumpFloor = os_AsmPrgmSize; // must happen before any calls to bump_free / bump_malloc;
    //SET_FUNC_BREAKPOINT(bump_free);
    // sadly ez80 clang can't seem to legalize taking the address of a label?
    //SET_LABEL_BREAKPOINT(sign_off);

    debug_printf("bump floor: %zu\n", bumpFloor);
	int errcode = 0;
    /*try*/ {
        IndirectBumpPointer<uint8, CONFIG_ROM_MEM_PTR> romManager(ROM_BYTES);
        //rom_mem = BumpPointer<uint8>(182184); // random number larger than user mem to simulate OOM crash
        {
            // find the ROM address ( in RAM other otherwise ;) )
            const TIFile romHandle{"RustlROM", "r"};
            const size_t codeSize = ti_GetSize(romHandle);
            debug_printf("Opened ROM: %zu bytes\n", codeSize);
            if (codeSize > ROM_BYTES) {
                error("<main>", "ROM size");
            } else {
                const void* progData = ti_GetDataPtr(romHandle);
                debug_printf("Copying %zu bytes from program (%p) to ROM (%p)\n", codeSize, progData, (void*)romManager.get());
                memcpy(romManager.get(), progData, codeSize);
                debug_printf("Setting remaing %zu bytes at %p to 0xff\n", (ROM_BYTES - codeSize), (void*)(romManager.get() + codeSize));
                memset(romManager.get() + codeSize, 0xff, ROM_BYTES - codeSize);
            }
        }

        if (rom_get (CODE_START+0) != 0xfb ||
            rom_get (CODE_START+1) != 0xd7) {
            printf ("*** The hex file was not compiled with PICOBIT\n");
        } else {
            ENTER_DEBUGGER();
            interpreter ();

#ifdef CONFIG_GC_STATISTICS
#ifdef CONFIG_GC_DEBUG
            printf ("**************** memory needed = %d\n", max_live + 1);
#endif
#endif
        }
    }/* catch (const std::exception &e) {
        debug_printf("Caught %s: %s\n", typeid(e).name(), e.what());
    } catch(...) {
        debug_printf("Caught unknown object - not an exception. Yikes!\n");
    }*/

    debug_printf("Exiting Rustle\n");
    ENTER_DEBUGGER();
	return errcode;
}
