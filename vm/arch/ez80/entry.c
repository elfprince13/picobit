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

uint8 ram_mem[RAM_BYTES + VEC_BYTES] = {0};
uint8 * rom_mem = NULL;

void* bump_malloc(const size_t size) {
    void* result = (os_UserMem + os_AsmPrgmSize);
    
    size_t bytesAvail = 0;
    asm volatile (
        "call\t0204FCh\n" // MemC
        : "=l" (bytesAvail)
        :
        : "c"
    );

    if (bytesAvail >= size) {

        asm volatile (
            "call\t020514h\n" // InsertMem
            : "=e" (result) // output - must use lower byte name for registers
            : "e"(size), "l"(result) // input - must use lower byte name for registers
            : "a", "c", "cc", "memory" // clobbers - must use lower byte name for registers (except a. cc is flags) - presumably "l" should also be here but clang doesn't like that because it's an input?
        );

        os_AsmPrgmSize += size;
        return result;
    } else {
        return NULL;
    }
}

void error (char *prim, char *msg)
{
    // maximum of 12 bytes, must be nul-terminated
    snprintf(os_AppErr1, 12, "%s:%s", prim, msg); 
    os_ThrowError(OS_E_APPERR1);
}

void type_error (char *prim, char *type)
{
	// maximum of 12 bytes, must be nul-terminated
    snprintf(os_AppErr2, 12, "%s-%s", prim, type);
    os_ThrowError(OS_E_APPERR2);
}

int main (void)
{
	int errcode = 0;
    {
        // find the ROM address ( in RAM other otherwise ;) )
        const uint8_t romHandle = ti_Open("RustlROM", "r");
        if (romHandle != 0) {
            // this isn't safe if we add support for manipulating TI-OS vars
            rom_mem = ti_GetDataPtr(romHandle);
            ti_Close(romHandle);
        } else {
            error("<main>", "ROM fail");
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

	return errcode;
}
