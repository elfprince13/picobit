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
#include <ti/flags.h>
#include <fileioc.h>
#include <debug.h>

#include <arch/exit-codes.h>
#include <arch/menu.h>

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
        std::exit(ExitError);//throw scheme_error(prim, msg);
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
        std::exit(ExitTypeError);//throw scheme_type_error(prim, type);
        /*
        // maximum of 12 bytes, must be nul-terminated
        snprintf(os_AppErr2, 12, "%s-%s", prim, type);
        os_ThrowError(OS_E_APPERR2);
        */
    }
}

bool loadBytecode(uint8_t * const rom, const uint8_t * const srcFile, const uint8_t * const srcFileEnd) {
    bool result = false;
    /*
    // misleading name: - really just hi8 because we would overflow our pointer size,
    // but keeping it this way for now to minimize changeset
	uint16_t hi16 = 0;
    */
    static_assert(CODE_START == 0, "non-zero code start requires re-enabling the hi16 codepath");

    const uint8_t * readHead = srcFile;
    while(readHead < srcFileEnd) {
        const uint8_t len = *(readHead++);
        if(readHead == srcFileEnd) { break; }
        const uint8_t a1 = *(readHead++);
        if(readHead == srcFileEnd) { break; }
        const uint8_t a2 = *(readHead++);
        if(readHead == srcFileEnd) { break; }
        const uint8_t t = *(readHead++);
        if(readHead == srcFileEnd) { break; }
        
        uint16_t a = ((uint16_t)a1 << 8) + a2;
        uint8_t sum = len + a1 + a2 + t;

        debug_printf("Line type %hhu, insert %02hhx bytes @ %04hx\n", t, len, a);
        switch (t) {
            case 0: {
                for (uint8_t i = 0; i < len; ++i) {
                    const uint16_t adr = a;//((uint24_t)hi16 << 16) + a - CODE_START;

                    const uint8_t b = *(readHead++);
                    if(readHead == srcFileEnd) { goto super_break; }
                    

                    // unsigned type is always positive!
                    if (adr < ROM_BYTES) {
                        rom[adr] = b;
                    } else {
                        printf("Address %04hx > ROM size %04hx, can't load bytecode\n", adr, (uint16_t)ROM_BYTES);
                        goto super_break;
                    }


                    a = (a + 1);// & 0xffff;
                    sum += b;
                }
            } break; 
            case 1: {
                if (len != 0) {
                    debug_printf("line type 1 must have length 0, got %hhx\n", len);
                    goto super_break;;
                }
            } break;
            case 4: {
                debug_printf("This bytecode was compiled for an unsupported extended address space!\n");
                std::exit(ExitDevAssertFailed);
                /*
                if (len != 2) {
                    break;
                }

                if ((a1 = read_hex_byte (f)) < 0 ||
                    (a2 = read_hex_byte (f)) < 0) {
                    break;
                }

                sum += a1 + a2;

                hi16 = (a1<<8) + a2;
                */
            } goto super_break;
            default: {
                debug_printf("Unknown line type %hhx\n", t);
            } goto super_break;
        }

        const uint8_t check = *(readHead++);
        if(readHead == srcFileEnd) { debug_printf("reached end of data stream, no more reads allowed!\n"); }
        
        // simulate negation on unsigned type.
        // can't make it a signed type because we need overflow which is U.B.
        sum = (1 + ~sum);

        if (sum != check) {
            debug_printf ("*** BIN file checksum error\n    (got %02hhx, expected 0x%02hhx)\n", sum, check);
            break;
        }
        /*
        c = *(readHead++);
        if(readHead == srcFileEnd) { break; }
        */
        if (t == 1) {
            debug_printf("Finished reading - success!\n");
            result = true;
            break;
        }
    }
super_break:

    if (!result) {
        printf ("*** BIN file syntax error\n");
    }

	return result;
}

int main ()
{
    
    std::atexit(&CleanupHook::cleanup);
    std::at_quick_exit(&CleanupHook::cleanup);
    
    bump_init();
    //SET_FUNC_BREAKPOINT(bump_free);
    // sadly ez80 clang can't seem to legalize taking the address of a label?
    //SET_LABEL_BREAKPOINT(sign_off);

	RustleExitStatus errcode = ExitSuccess;
    /*try*/ {
        IndirectBumpPointer<uint8, CONFIG_ROM_MEM_PTR> romManager(ROM_BYTES);
    
        {
            const char* romName;
            void* romVAT = nullptr;
            debug_printf("Scanning for ROMs\n");
            char availableRoms[15][9] = {{0}}; 
            uint8_t numRoms;
            for(numRoms = 0; numRoms < 15 && (romName = ti_Detect(&romVAT, "\xd7\xfb")); ++numRoms) {
                debug_printf("Found Rustle ROM image: %s\n", romName);
                strcpy(availableRoms[numRoms], romName);
            }
            if (numRoms == 15) {
                debug_printf("15 ROMS found but menu can only handle 15 selections currently...some results may be missing\n");
            }
            const uint8_t selected = fetchMenu("Select compiled program:", availableRoms, numRoms);
            if (selected >= numRoms) {
                if (selected > numRoms) {
                    debug_printf("fetchMenu broken!! %hhu >= %hhu\n", selected, numRoms);
                    std::exit(ExitDevAssertFailed);
                } else {
                    std::exit(ExitSuccess); // User quit the program
                }
            }
            // find the ROM address ( in RAM other otherwise ;) )
            const TIFile romHandle{ availableRoms[selected] /*"RustlROM"*/, "r"};
            const size_t fileSize = ti_GetSize(romHandle);

            if (fileSize <= 2) {
                error("<main>", "Invalid file!");
            }

            const size_t codeSize = fileSize - 2;
            
            debug_printf("Opened ROM: %zu bytes\n", codeSize);
            if (codeSize > ROM_BYTES) {
                error("<main>", "ROM size");
            } else {
                // skip the header.
                const uint8_t* progData = ((const uint8_t *)ti_GetDataPtr(romHandle)) + 2;
                memset(romManager.get(), 0xff, ROM_BYTES);
                if (!loadBytecode(romManager.get(), progData, progData + codeSize)) {
                    error("<main>", "Failed to load bytecode");
                }
                /*
                debug_printf("Copying %zu bytes from program (%p) to ROM (%p)\n", codeSize, progData, (void*)romManager.get());
                memcpy(romManager.get(), progData, codeSize);
                debug_printf("Setting remaing %zu bytes at %p to 0xff\n", (ROM_BYTES - codeSize), (void*)(romManager.get() + codeSize));
                memset(romManager.get() + codeSize, 0xff, ROM_BYTES - codeSize);
                //*/
            }
        }
        os_SetFlag(APP, AUTOSCROLL);
        if ((rom_get (CODE_START+0) & 0xf8) != 0xd8 ||
            rom_get (CODE_START+1) != 0xfb) {
            // should never reach this code if above successfully filters
            printf ("*** The hex file was not compiled with PICOBIT\n");
            errcode = ExitDevAssertFailed;
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
    // avoid drawing garbage on the screen when we exit.
    memset(ram_mem, 0x00, RAM_BYTES + VEC_BYTES);
    debug_printf("Exiting Rustle\n");
	return errcode;
}
