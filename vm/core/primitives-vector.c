#include <picobit.h>
#include <bignum.h>
#include <primitives.h>
#include <gc.h>
#include <string.h>
#include <debug.h>

PRIMITIVE(u8vector?, u8vector_p, 1)
{
	if (IN_RAM(arg1)) {
		arg1 = encode_bool (RAM_U8VECTOR_P(arg1));
	} else if (IN_ROM(arg1)) {
		arg1 = encode_bool (ROM_U8VECTOR_P(arg1));
	} else {
		arg1 = OBJ_FALSE;
	}
}

PRIMITIVE(#%make-u8vector, make_u8vector, 2)
{
	a1 = decode_int (arg1); // arg1 is length
	// TODO adapt for the new bignums
	a2 = decode_int (arg2); // arg2 is fill-number
	if (a2 > 255) {
		TYPE_ERROR("#%make-vector.0","arg2 not 0 < x <= 255");
	}

	arg1 = alloc_ram_cell_init (COMPOSITE_FIELD0 | (a1 >> 8),
	                            a1 & 0xff,
	                            U8VECTOR_FIELD2,
	                            0); // will be filled in later
	arg2 = alloc_vec_cell (a1, arg1);
	ram_set_cdr(arg1, arg2);
	a3 = VEC_TO_RAM_BASE_ADDR(arg2);
	// TODO: very janky, bypasses helper function, do not like:
	IF_TRACE(debug_printf("Writing %zu of %u to %p = (%p + %zu) (vec cell %hu)\n", (size_t)a1, (int)a2, (void*)(ram_mem + a3), (void*)ram_mem, (size_t)a3, arg2));
	memset(ram_mem + a3, a2, a1);
	arg2 = OBJ_FALSE;
}

PRIMITIVE(u8vector-ref, u8vector_ref, 2)
{
	a2 = decode_int (arg2);
	arg2 = OBJ_FALSE;

	// TODO adapt for the new bignums
	if (IN_RAM(arg1)) {
		if (!RAM_U8VECTOR_P(arg1)) {
			TYPE_ERROR("u8vector-ref.0", "vector");
		}

		if (ram_get_car (arg1) <= a2) {
			ERROR("u8vector-ref.0", "vector index invalid");
		}
		arg1 = ram_get_cdr (arg1);
		arg1 = VEC_TO_RAM_BASE_ADDR(arg1);
		arg1 = ram_get(arg1 + a2);
	} else if (IN_ROM(arg1)) {
		if (!ROM_U8VECTOR_P(arg1)) {
			TYPE_ERROR("u8vector-ref.1", "vector");
		}

		if (rom_get_car (arg1) <= a2) {
			ERROR("u8vector-ref.1", "vector index invalid");
		}
		arg1 = rom_get_cdr (arg1);
		arg1 = VEC_TO_ROM_BASE_ADDR(arg1);
		arg1 = rom_get(arg1 + a2);
	} else {
		TYPE_ERROR("u8vector-ref.2", "vector");
	}

	arg1 = encode_int (arg1);
}

PRIMITIVE_UNSPEC(u8vector-set!, u8vector_set, 3)
	// TODO a lot in common with ref, abstract that
{
	a2 = decode_int (arg2); // TODO adapt for bignums
	a3 = decode_int (arg3);

	if (a3 > 255) {
		ERROR("u8vector-set!", "byte vectors can only contain bytes");
	}

	if (IN_RAM(arg1)) {
		if (!RAM_U8VECTOR_P(arg1)) {
			TYPE_ERROR("u8vector-set!.0", "vector");
		}

		if (ram_get_car (arg1) <= a2) {
			ERROR("u8vector-set!", "vector index invalid");
		}

		arg1 = VEC_TO_RAM_BASE_ADDR(ram_get_cdr (arg1));
		a1 = (arg1 >> 2) - 1; // the vector header
		if (ram_get((a1 << 2) + FIELD0_OFFSET) & 0x80) {
			TYPE_ERROR("u8vector-set!.1", "mu8vector");
		}
	} else {
		TYPE_ERROR("u8vector-set!.2", "vector");
	}

	ram_set (arg1 + a2, a3);

	arg1 = OBJ_FALSE;
	arg2 = OBJ_FALSE;
	arg3 = OBJ_FALSE;
}

PRIMITIVE(u8vector-length, u8vector_length, 1)
{
	if (IN_RAM(arg1)) {
		if (!RAM_U8VECTOR_P(arg1)) {
			TYPE_ERROR("u8vector-length.0", "vector");
		}

		arg1 = encode_int (ram_get_car (arg1));
	} else if (IN_ROM(arg1)) {
		if (!ROM_U8VECTOR_P(arg1)) {
			TYPE_ERROR("u8vector-length.1", "vector");
		}

		arg1 = encode_int (rom_get_car (arg1));
	} else {
		TYPE_ERROR("u8vector-length.2", "vector");
	}
}
