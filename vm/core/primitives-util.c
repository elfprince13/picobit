#include <picobit.h>
#include <primitives.h>
#include <gc.h>
#include <string.h>

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

PRIMITIVE(symbol->string, symbol2string, 1)
{
	if (IN_RAM(arg1) && RAM_SYMBOL_P(arg1)) {
		arg1 = ram_get_car(arg1);
	} else if (IN_ROM(arg1) && ROM_SYMBOL_P(arg1)) {
		arg1 = rom_get_car(arg1);
	} else {
		type_error("symbol->string", "symbol");
	}
}

// TODO: this doesn't handle intern-ing yet!
// we need object vectors to properly implement
// the intern table
PRIMITIVE(string->symbol, string2symbol, 1)
{
	uint8 * srcPointer;
	if (IN_RAM(arg1) && RAM_STRING_P(arg1)) {
		a1 = ram_get_car(arg1);
		srcPointer = ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(arg1));
	} else if (IN_ROM(arg1) && ROM_STRING_P(arg1)) {
		a1 = rom_get_car(arg1);
		srcPointer = rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(arg1));
	} else {
		type_error("symbol->string", "symbol");
	}

	arg2 = alloc_ram_cell_init (COMPOSITE_FIELD0 | (a1 >> 8),
	                            a1 & 0xff,
	                            STRING_FIELD2,
	                            0); // will be filled in later
	arg1 = alloc_vec_cell (1 + a1, arg2); // leave room to nul-terminate
	ram_set_cdr(arg2, arg1);
	a2 = VEC_TO_RAM_BASE_ADDR(arg1);
	uint8 * dstPointer = ram_mem + a2;
	memcpy(dstPointer, srcPointer, 1 + a1);

	a2 = (((arg1 >> 2) - 1) << 2) + FIELD0_OFFSET; // the vector header
	ram_set(a2, ram_get(a2) | 0x80); // mark as immutable

	arg1 = alloc_ram_cell_init (COMPOSITE_FIELD0, 0, SYMBOL_FIELD2, 0);
	ram_set_car(arg1, arg2);
	//arg1 = arg2;
	arg2 = OBJ_FALSE;
}

PRIMITIVE(boolean?, boolean_p, 1)
{
	arg1 = encode_bool (arg1 < 2);
}
