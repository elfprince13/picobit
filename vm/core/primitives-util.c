#include <picobit.h>
#include <debug.h>
#include <primitives.h>
#include <gc.h>
#include <string.h>
#include <symtable.h>

PRIMITIVE(eq?, eq_p, 2)
{
	arg1 = encode_bool (arg1 == arg2);
	arg2 = OBJ_FALSE;
}

PRIMITIVE(not, not, 1)
{
	arg1 = encode_bool (arg1 == OBJ_FALSE);
}

PRIMITIVE(symbol?, symbol_p, 1)
{
	if (IN_RAM(arg1)) {
		arg1 = encode_bool (RAM_SYMBOL_P(arg1));
	} else if (IN_ROM(arg1)) {
		arg1 = encode_bool (ROM_SYMBOL_P(arg1));
	} else {
		arg1 = OBJ_FALSE;
	}
}

PRIMITIVE(symbol->immutable-string, symbol2immutablestring, 1)
{
	if (IN_RAM(arg1) && RAM_SYMBOL_P(arg1)) {
		arg1 = ram_get_car(arg1);
	} else if (IN_ROM(arg1) && ROM_SYMBOL_P(arg1)) {
		arg1 = rom_get_car(arg1);
	} else {
		type_error("symbol->string", "symbol");
	}
}

PRIMITIVE(string->uninterned-symbol, string2uninternedsymbol, 1)
{
	const uint8 * srcPointer;
	if (IN_RAM(arg1) && RAM_STRING_P(arg1)) {
		a1 = ram_get_car(arg1);
		srcPointer = ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(arg1));
	} else if (IN_ROM(arg1) && ROM_STRING_P(arg1)) {
		a1 = rom_get_car(arg1);
		srcPointer = rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(arg1));
	} else {
		type_error("symbol->string", "symbol");
	}

	const obj header = alloc_vec_cell (1 + a1); // leave room to nul-terminate
	a3  = _SYS_RAM_TO_VEC_OBJ(header + 1);;
	arg2 = alloc_ram_cell_init (COMPOSITE_FIELD0 | (a1 >> 8),
	                            a1 & 0xff,
	                            STRING_FIELD2 | (a3 >> 8),
	                            a3);
	ram_set_cdr(header, arg2); // tie the knot
	a2 = VEC_TO_RAM_BASE_ADDR(a3);
	uint8 * dstPointer = ram_mem + a2;
	memcpy(dstPointer, srcPointer, 1 + a1);

	a2 = (((arg1 >> 2) - 1) << 2) + FIELD0_OFFSET; // the vector header
	ram_set(a2, ram_get(a2) | 0x80); // mark as immutable

	arg1 = alloc_ram_cell_init (COMPOSITE_FIELD0, 0, SYMBOL_FIELD2, 0);
	ram_set_car(arg1, arg2);
	arg2 = OBJ_FALSE;
}


PRIMITIVE(string->symbol, string2symbol, 1)
{
	prim_string2uninternedsymbol();
	// clobbers arg1 + arg2
	const obj retVal = intern_symbol(arg1);
	if (symTableSize > symTableBuckets) {
		increase_sym_table_capacity();
	} else {
            IF_TRACE((debug_printf("symTable: "), show_obj(symTable), debug_printf("\n")));
	}
	arg1 = retVal;
}

PRIMITIVE(boolean?, boolean_p, 1)
{
	arg1 = encode_bool (arg1 < 2);
}
