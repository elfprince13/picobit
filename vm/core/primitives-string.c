#include <picobit.h>
#include <bignum.h>
#include <primitives.h>
#include <gc.h>
#include <string.h>

// TODO: file is heavily duplicative of vector file

PRIMITIVE(string?, string_p, 1)
{
	if (IN_RAM(arg1)) {
		arg1 = encode_bool (RAM_STRING_P(arg1));
	} else if (IN_ROM(arg1)) {
		arg1 = encode_bool (ROM_STRING_P(arg1));
	} else {
		arg1 = OBJ_FALSE;
	}
}

PRIMITIVE(#%make-string, make_string, 2)
{
	a1 = decode_int (arg1); // arg1 is length
	// TODO adapt for the new bignums
	a2 = decode_int (arg2); // arg2 is fill-character
	if (a2 > 255) {
		TYPE_ERROR("#%make-string.0","arg2 not 0 < c <= 255");
	}

	arg1 = alloc_ram_cell_init (COMPOSITE_FIELD0 | (a1 >> 8),
	                            a1 & 0xff,
	                            STRING_FIELD2,
	                            0); // will be filled in later
	arg2 = alloc_vec_cell (1 + a1, arg1); // leave room to nul-terminate
	ram_set_cdr(arg1, arg2);
	arg2 = VEC_TO_RAM_BASE_ADDR(arg2);
	// TODO: very janky, bypasses helper function, do not like:
	memset(ram_mem + arg2, a2, a1);
	ram_set(arg2 + a1, 0); // nul-terminate
	arg2 = OBJ_FALSE;
}

PRIMITIVE(string-ref, string_ref, 2)
{
	a2 = decode_int (arg2);

	// TODO adapt for the new bignums
	if (IN_RAM(arg1)) {
		if (!RAM_STRING_P(arg1)) {
			TYPE_ERROR("string-ref.0", "string");
		}

		if (ram_get_car (arg1) <= a2) {
			ERROR("string-ref.0", "string index invalid");
		}
		arg1 = ram_get_cdr (arg1);
		arg1 = VEC_TO_RAM_BASE_ADDR(arg1);
		arg1 = ram_get(arg1 + arg2);
	} else if (IN_ROM(arg1)) {
		if (!ROM_STRING_P(arg1)) {
			TYPE_ERROR("string-ref.1", "string");
		}

		if (rom_get_car (arg1) <= a2) {
			ERROR("string-ref.1", "string index invalid");
		}
		arg1 = rom_get_cdr (arg1);
		arg1 = VEC_TO_ROM_BASE_ADDR(arg1);
		arg1 = rom_get(arg1 + arg2);
	} else {
		TYPE_ERROR("string-ref.2", "string");
	}

	arg1 = encode_int (arg1);

	arg2 = OBJ_FALSE;
}

PRIMITIVE_UNSPEC(string-set!, string_set, 3)
	// TODO a lot in common with ref, abstract that
{
	a2 = decode_int (arg2); // TODO adapt for bignums
	a3 = decode_int (arg3);

	if (a3 > 255) {
		ERROR("string-set!", "strings can only contain bytes");
	}

	if (IN_RAM(arg1)) {
		if (!RAM_STRING_P(arg1)) {
			TYPE_ERROR("string-set!.0", "string");
		}

		if (ram_get_car (arg1) <= a2) {
			ERROR("string-set!", "string index invalid");
		}

		arg1 = VEC_TO_RAM_BASE_ADDR(ram_get_cdr (arg1));
	} else {
		TYPE_ERROR("string-set!.1", "string");
	}

	ram_set (arg1 + a2, a3);

	arg1 = OBJ_FALSE;
	arg2 = OBJ_FALSE;
	arg3 = OBJ_FALSE;
}

PRIMITIVE(string-length, string_length, 1)
{
	if (IN_RAM(arg1)) {
		if (!RAM_STRING_P(arg1)) {
			TYPE_ERROR("string-length.0", "string");
		}

		arg1 = encode_int (ram_get_car (arg1));
	} else if (IN_ROM(arg1)) {
		if (!ROM_STRING_P(arg1)) {
			TYPE_ERROR("string-length.1", "string");
		}

		arg1 = encode_int (rom_get_car (arg1));
	} else {
		TYPE_ERROR("string-length.2", "string");
	}
}


PRIMITIVE(string->list, string2list, 1)
{
	if (IN_RAM(arg1)) {
		if (!RAM_STRING_P(arg1)) {
			TYPE_ERROR("string->list.0", "string");
		}
		a1 = ram_get_car (arg1); // length - encoded directly
		arg1 = VEC_TO_RAM_BASE_ADDR(ram_get_cdr (arg1));
		a2 = OBJ_NULL;
		while (a1) {
			a1 -= 1;
			a2 = cons(encode_int(ram_get(arg1 + a1)), a2);
		}
		arg1 = ram_get_car (arg1);
	} else if (IN_ROM(arg1)) {
		if (!ROM_STRING_P(arg1)) {
			TYPE_ERROR("string->list.1", "string");
		}
		a1 = rom_get_car (arg1); // length - encoded directly
		arg1 = VEC_TO_ROM_BASE_ADDR(rom_get_cdr (arg1));
		a2 = OBJ_NULL;
		while (a1) {
			a1 -= 1;
			a2 = cons(encode_int(rom_get(arg1 + a1)), a2);
		}
		arg1 = rom_get_car (arg1);
	} else {
		TYPE_ERROR("string->list.2", "string");
	}
}

PRIMITIVE(list->string, list2string, 1)
{
	arg2 = arg1;
	a1 = 0; // length accumulator
	while (arg2 != OBJ_NULL) {
		if (IN_RAM(arg2)) {
			if (!RAM_STRING_P(arg2)) {
				TYPE_ERROR("list->string.0", "pair");
			}
			arg2 = ram_get_cdr(arg2);
		} else if (IN_ROM(arg2)) {
			if (!ROM_STRING_P(arg2)) {
				TYPE_ERROR("list->string.0", "pair");
			}
			arg2 = rom_get_cdr(arg2);
		} else {
			TYPE_ERROR("list->string.1", "pair");
		}
		++a1;
	}
	arg2 = arg1;
	arg1 = alloc_ram_cell_init (COMPOSITE_FIELD0 | (a1 >> 8),
	                            a1 & 0xff,
	                            STRING_FIELD2,
	                            0); // will be filled in later
	a2 = alloc_vec_cell (1 + a1, arg1); // leave room to nul-terminate
	ram_set_cdr(arg1, a2);
	a2 = VEC_TO_RAM_BASE_ADDR(a2);
	a1 = 0; // index accumulator
	while (arg2 != OBJ_NULL) {
		if (IN_RAM(arg2)) {
			if (!RAM_STRING_P(arg2)) {
				TYPE_ERROR("list->string.2", "pair");
			}
			a3 = ram_get_car(arg2);
			arg2 = ram_get_cdr(arg2);
		} else if (IN_ROM(arg2)) {
			if (!ROM_STRING_P(arg2)) {
				TYPE_ERROR("list->string.2", "pair");
			}
			a3 = rom_get_car(arg2);
			arg2 = rom_get_cdr(arg2);
		} else {
			TYPE_ERROR("list->string.3", "pair");
		}

		if(a3 > 255) {
			TYPE_ERROR("list->string.4", "char");
		}
		ram_set(a2 + a1, a3);
		++a1;
	}
	ram_set(a2 + a1, 0);
	arg2 = OBJ_FALSE;
}