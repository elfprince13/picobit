#ifndef PICOBIT_PICOBIT_H
#define PICOBIT_PICOBIT_H
#include <generated/autoconf.h>

#include <arch/types.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Picobit complex types */

typedef uint8 word;

typedef uint16 ram_addr;
typedef uint16 rom_addr;

typedef uint16 obj; /* Only 13 bits are used in pointers */

#if defined(__GNUC__) || defined(__clang__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif /* defined(__GNUC__) */

#ifdef __cplusplus
}
#endif

#include <heap.h>
#include <object.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GC roots set. */

extern obj cont, env;
extern obj arg1, arg2, arg3, arg4;
extern obj symTable;

/* Interpreter variables. */

extern rom_addr pc, entry;
extern uint8 glovars;

#ifdef CONFIG_ERROR_HANDLING

#define ERROR(prim, msg) error (prim, msg)
#define TYPE_ERROR(prim, type) type_error (prim, type)
void error (const char *prim, const char *msg) NORETURN;
void type_error (const char *prim, const char *type) NORETURN;

#else

void halt_with_error () NORETURN;
#define ERROR(prim, msg) halt_with_error()
#define TYPE_ERROR(prim, type) halt_with_error()

#endif /* CONFIG_ERROR_HANDLING */
#ifdef __cplusplus
}
#endif
#endif
