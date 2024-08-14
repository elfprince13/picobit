#ifndef PICOBIT_DEBUG_H
#define PICOBIT_DEBUG_H

#include <generated/autoconf.h>

#if defined(CONFIG_VM_DEBUG) || defined(CONFIG_GC_DEBUG) || defined(CONFIG_DEBUG_STRINGS)
#include <stdio.h>
#ifdef CONFIG_CUSTOM_DEBUG_PRINTF
#ifndef debug_printf
#error CUSTOM_DEBUG_PRINTF is selected in configuration but no debug_printf was defined.
#endif
#else
#ifdef debug_printf
#error CUSTOM_DEBUG_PRINTF is not selected in configuration but debug_printf was defined.
#endif
#define debug_printf printf
#endif
#else
#ifdef debug_printf
#undef debug_printf
#endif
#define debug_printf(...)
#endif

#ifdef CONFIG_VM_DEBUG
#define IF_TRACE(x) x
#else
#define IF_TRACE(x)
#endif

#ifdef CONFIG_GC_DEBUG
#define IF_GC_TRACE(x) x
#else
#define IF_GC_TRACE(x)
#endif

#ifdef CONFIG_DEBUG_STRINGS
#ifdef __cplusplus
extern "C" {
#endif
void show_type (obj o);
void show_obj (obj o);
void show_state (rom_addr pc);
#ifdef __cplusplus
}
#endif
#else
#define show_type(o)
#define show_obj(o)
#define show_state(pc)
#endif
#endif
