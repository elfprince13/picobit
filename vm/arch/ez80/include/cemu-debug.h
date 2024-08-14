#ifndef PICOBIT_ARCH_EZ80_CEMU_DEBUG_H
#define PICOBIT_ARCH_EZ80_CEMU_DEBUG_H

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

#endif