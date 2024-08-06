#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <picobit.h>
#include <dispatch.h>
#include <gc.h>

#include <ti/error.h>
#include <fileioc.h>
#include <debug.h>

#define os_UserMem ((void*)0xD1A881)

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

uint8 ram_mem[RAM_BYTES + VEC_BYTES] = {0};
uint8 * rom_mem = NULL;

static uint24_t bumpFloor;

void* bump_malloc(const size_t size) {
    debug_printf("bump_malloc: %zu bytes requested\n", size);
    
    size_t bytesAvail = 0;
    asm volatile (
        "call\t0204FCh\n" // MemC
        : "=l" (bytesAvail)
        :
        : "c"
    );

    debug_printf("bump_malloc: %zu bytes available\n", bytesAvail);

    if (bytesAvail >= size) {
        void* result = (os_UserMem + os_AsmPrgmSize);
        debug_printf("InsertMem at %p\n", result);
        asm volatile (
            "call\t020514h\n" // InsertMem
            : "=e" (result) // output - must use lower byte name for registers
            : "e"(result), "l"(size) // input - must use lower byte name for registers
            : "a", "c", "cc", "memory" // clobbers - must use lower byte name for registers (except a. cc is flags) - presumably "l" should also be here but clang doesn't like that because it's an input?
        );
        debug_printf("Updating reserved os_AsmPrgmSize\n");
        os_AsmPrgmSize += size;
        return result;
    } else {
        debug_printf("Not enough memory!\n");
        return NULL;
    }
}

void bump_free(void * ptr) {
    const void * curCeiling = (os_UserMem + os_AsmPrgmSize);
    if (ptr < curCeiling) {
        if (ptr >= (os_UserMem + bumpFloor)) {
            const size_t size = curCeiling - ptr;
            asm volatile (
                "call\t020590h\n" // DelMem
                : 
                : "l"(ptr), "e"(size)
                : "a", "c", "cc", "memory"
            );
            os_AsmPrgmSize -= size;
        } else {
            error("bump", "bad free");
        }
    } else {
        error("bump", "re-free");
    }
}

void error (char *prim, char *msg)
{
    if (rom_mem != NULL) {
        void* cleanup = rom_mem;
        rom_mem = NULL;
        bump_free(cleanup);
    }
    // maximum of 12 bytes, must be nul-terminated
    snprintf(os_AppErr1, 12, "%s:%s", prim, msg); 
    os_ThrowError(OS_E_APPERR1);
}

void type_error (char *prim, char *type)
{
    if (rom_mem != NULL) {
        void* cleanup = rom_mem;
        rom_mem = NULL;
        bump_free(cleanup);
    }
	// maximum of 12 bytes, must be nul-terminated
    snprintf(os_AppErr2, 12, "%s-%s", prim, type);
    os_ThrowError(OS_E_APPERR2);
}

int main (void)
{
    bumpFloor = os_AsmPrgmSize; // must happen before any calls to bump_free / bump_malloc;

    debug_printf("bump floor: %zu\n", bumpFloor);
	int errcode = 0;
    {
        // find the ROM address ( in RAM other otherwise ;) )
        const uint8_t romHandle = ti_Open("RustlROM", "r");
        if (romHandle != 0) {
            const size_t codeSize = ti_GetSize(romHandle);
            debug_printf("Opened ROM: %zu bytes\n", codeSize);
            if (codeSize > ROM_BYTES) {
                ti_Close(romHandle);
                error("<main>", "ROM size");
            } else {
                rom_mem = bump_malloc(ROM_BYTES);
                const void* progData = ti_GetDataPtr(romHandle);
                debug_printf("Copying %zu bytes from program (%p) to ROM (%p)\n", codeSize, progData, (void*)rom_mem);
                memcpy(rom_mem, progData, codeSize);
                debug_printf("Setting remaing %zu bytes at %p to 0xff\n", (ROM_BYTES - codeSize), (void*)(rom_mem + codeSize));
                memset(rom_mem + codeSize, 0xff, ROM_BYTES - codeSize);
                ti_Close(romHandle);
            }
        } else {
            error("<main>", "ROM fail");
        }
    }

    if (rom_get (CODE_START+0) != 0xfb ||
        rom_get (CODE_START+1) != 0xd7) {
        printf ("*** The hex file was not compiled with PICOBIT\n");
    } else {
        SET_FUNC_BREAKPOINT(show_state);
        interpreter ();

#ifdef CONFIG_GC_STATISTICS
#ifdef CONFIG_GC_DEBUG
        printf ("**************** memory needed = %d\n", max_live + 1);
#endif
#endif
    }

    if (rom_mem != NULL) {
        void* cleanup = rom_mem;
        rom_mem = NULL;
        bump_free(cleanup);
    }

	return errcode;
}
