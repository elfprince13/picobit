core-y += dispatch.c heap.c gc.c symtable.c \
	primitives-list.c primitives-numeric.c \
	primitives-string.c \
	primitives-util.c primitives-vector.c \
	primitives-control.c

core-$(CONFIG_BIGNUM_LONG)   += bignum_long.c
core-$(CONFIG_BIGNUM_FIXED)  += bignum_fixed.c
core-$(CONFIG_DEBUG_STRINGS) += debug.c

core-h := include/bignum.h include/debug.h include/dispatch.h \
	include/escapes.h include/gc.h include/gen.primitives.h \
	include/heap.h include/object.h include/picobit.h include/primitives.h \
	include/symtable.h