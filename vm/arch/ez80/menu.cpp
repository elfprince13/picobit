#include <picobit.h>
#include <arch/menu.h>
#include <ti/screen.h>
#include <ti/getcsc.h>
#include <debug.h>

#include <arch/exit-codes.h>


#define os_FlagsIY ((uint8_t*)0xD00080)

extern "C" {
    void outchar(const char disp) {
        if ('\n' == disp) {
            os_NewLine();
        } else {
            asm volatile (
            "call\t0207B8h\n" // PutC
            :
            : "a"(disp), "iyl"(os_FlagsIY)
            :
        );
        }
    }
}

uint8_t fetchMenu(const char* const title, const uint8_t nOptions, const char** const options) {
    uint8_t retVal = nOptions;
    font_t * const backupFont = os_FontGetID();
    os_ClrHomeFull();
    os_FontSelect(os_SmallFont);
    os_PutStrFull(title);
    os_NewLine();
    for(size_t i = 0; i < nOptions; ++i) {
        constexpr static const char shortcuts[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
        if (i < (sizeof(shortcuts) / sizeof(char))) {
            outchar(shortcuts[i]);
            os_PutStrFull(": ");
        }
        os_PutStrFull(options[i]);
        os_NewLine();
    }
    do {
        if (boot_CheckOnPressed()) {
            debug_printf("on pressed");
            std::exit(ExitTerminated); // abort!
        } else {
            uint8_t key;
            do {
                key = os_GetCSC();
            } while (!key);
            debug_printf("Keypress: %hhu\n", key);
            switch(key) { 
                case sk_Recip:// D
                case sk_Math: // A
                {
                    key -= 6; // E,F -> 8,9
                    debug_printf("key - 6\n");
                } [[fallthrough]];
                case sk_Sin:  // E
                case sk_Apps: // B
                {
                    key -= 1; // 6,7,8,9 -> 5,6,7,8
                    debug_printf("key - 1\n");
                } [[fallthrough]];
                case sk_1:    // 1
                case sk_4:    // 4
                case sk_7:    // 7
                {
                    // 2,3,4,5,6,7,8 -> 1,4,7,E,B,D,A -> 0,3,6,D,A,C,9
                    constexpr static const uint8_t key2hex[] = {0xF, 0xF, 0, 3, 6, 0xD, 0xA, 0xC, 0x9};
                    retVal = key2hex[key & 0xF];
                    debug_printf("key %hhu translated to %hhu\n", key, retVal);
                } break;

                case sk_Cos:  // F
                case sk_Prgm: // C
                {
                    key -= 1; // E, F -> D, E
                    debug_printf("key - 1\n");
                } [[fallthrough]];
                case sk_2:    // 2
                case sk_5:    // 5
                case sk_8:    // 8
                {
                    key -= 5; // A,B,C,D,E -> 5,6,7,8,9
                    debug_printf("key - 5\n");
                } [[fallthrough]];
                case sk_3:    // 3
                case sk_6:    // 6
                case sk_9:    // 9
                {
                    // 2,3,4,5,6,7,8,9 -> 3,6,9,2,5,8,F,C -> 2,5,8,1,4,7,E,B
                    constexpr static const uint8_t key2hex[] = {0xF, 0xF, 2, 5, 8, 1, 4, 5, 0xE, 0xB};
                    retVal = key2hex[key & 0xF];
                    debug_printf("key %hhu translated to %hhu\n", key, retVal);
                } break;
                
                case sk_Clear: {
                    debug_printf("CLEAR!!\n");
                    retVal = nOptions;
                    goto double_break;
                }
                default: /* ignore */;
            }
        }
    } while (nOptions <= retVal);
double_break:

    os_FontSelect(backupFont);
    os_ClrHomeFull();

    return retVal;
}