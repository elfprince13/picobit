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
        if (0 == handle_) {
            error("<TIFile>", "File did not exist (or could not be opened)");
        }
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
    void error (const char *prim, const char *msg)
    {
        debug_printf("Error! %s: %s\n", prim, msg);
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
        debug_printf("Type Error! %s: %s\n", prim, type);
        snprintf(os_AppErr2, 12, "%s-%s", prim, type);
        std::exit(-1);//throw scheme_type_error(prim, type);
        /*
        // maximum of 12 bytes, must be nul-terminated
        snprintf(os_AppErr2, 12, "%s-%s", prim, type);
        os_ThrowError(OS_E_APPERR2);
        */
    }
}

int main ()
{
    std::atexit(&CleanupHook::cleanup);
    std::at_quick_exit(&CleanupHook::cleanup);
    
    bump_init();
    //SET_FUNC_BREAKPOINT(bump_free);
    // sadly ez80 clang can't seem to legalize taking the address of a label?
    //SET_LABEL_BREAKPOINT(sign_off);

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
	return errcode;
}
