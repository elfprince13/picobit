#ifndef PICOBIT_SYMTABLE_H
#define PICOBIT_SYMTABLE_H
#include <picobit.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

inline unsigned int integer_log(unsigned int v) {
	// integer log adapted from https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
	// avoid branching
    register uint16_t r; // result of log2(v) will go here
	register uint16_t shift;

#ifdef UINT_MAX
#if UINT_MAX > 65535ULL
#if (UINT_MAX > 4294967295ULL) && (UINT_MAX <= 18446744073709551615ULL)
    r =     (v > 0xFFFFFFFF) << 5; v >>= r;
    shift = (v > 0xFFFF) << 4; v >>= shift; r |= shift;
#else
#if (UINT_MAX > 65535ULL) || (UINT_MAX <= 4294967295ULL)
    r =     (v > 0xFFFF) << 4; v >>= r;
#else
#define STRINGIFY(s) XSTRINGIFY(s)
#define XSTRINGIFY(s) #s
#error ("code must be extended for novel int sizes. UINT_MAX=" STRINGIFY(UINT_MAX))
#endif
#endif
    shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
#else
#if UINT_MAX == 65535ULL
    r =     (v > 0xFF  ) << 3; v >>= r;
#else
#define STRINGIFY(s) XSTRINGIFY(s)
#define XSTRINGIFY(s) #s
#error ("code must be extended for novel int sizes. UINT_MAX=" STRINGIFY(UINT_MAX))
#endif
#endif
#else
#error "Must include <limits.h> header with UINT_MAX definition"
#endif
	shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
	shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
											r |= (v >> 1);
    return r;
}

inline unsigned int npot(unsigned int v) {
    unsigned int r = integer_log((v - 1));
    return (unsigned int)1 << (unsigned int)(r + 1);
}

inline uint16 uhash_combine(uint16 hash, uint8 key) {
	return (uint16)((hash ^ key) + ((hash << 13) | (hash >> 3)));
}

uint16 hash_string_buffer(obj str);
void init_sym_table(uint8 numConstants);

#ifdef __cplusplus
}
#endif
#endif