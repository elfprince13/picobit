#include <picobit.h>
#include <bignum.h>
#include <primitives.h>
#include <gc.h>
#include <string.h>
#include <debug.h>

// u8vectors
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
			TYPE_ERROR("u8vector-ref.0", "u8vector");
		}

		if (ram_get_car (arg1) <= a2) {
			ERROR("u8vector-ref.0", "vector index invalid");
		}
		arg1 = ram_get_cdr (arg1);
		arg1 = VEC_TO_RAM_BASE_ADDR(arg1);
		arg1 = ram_get(arg1 + a2);
	} else if (IN_ROM(arg1)) {
		if (!ROM_U8VECTOR_P(arg1)) {
			TYPE_ERROR("u8vector-ref.1", "u8vector");
		}

		if (rom_get_car (arg1) <= a2) {
			ERROR("u8vector-ref.1", "vector index invalid");
		}
		arg1 = rom_get_cdr (arg1);
		arg1 = VEC_TO_ROM_BASE_ADDR(arg1);
		arg1 = rom_get(arg1 + a2);
	} else {
		TYPE_ERROR("u8vector-ref.2", "u8vector");
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
			TYPE_ERROR("u8vector-set!.0", "u8vector");
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
		TYPE_ERROR("u8vector-set!.2", "u8vector");
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
			TYPE_ERROR("u8vector-length.0", "u8vector");
		}

		arg1 = encode_int (ram_get_car (arg1));
	} else if (IN_ROM(arg1)) {
		if (!ROM_U8VECTOR_P(arg1)) {
			TYPE_ERROR("u8vector-length.1", "u8vector");
		}

		arg1 = encode_int (rom_get_car (arg1));
	} else {
		TYPE_ERROR("u8vector-length.2", "u8vector");
	}
}

//////////////////////////////////////////
//////////////////////////////////////////

PRIMITIVE(vector?, vector_p, 1)
{
	if (IN_RAM(arg1)) {
		arg1 = encode_bool (RAM_VECTOR_P(arg1));
	} else if (IN_ROM(arg1)) {
		arg1 = encode_bool (ROM_VECTOR_P(arg1));
	} else {
		arg1 = OBJ_FALSE;
	}
}

PRIMITIVE(#%make-vector, make_vector, 2)
{
	uint16 i;
	// TODO adapt for the new bignums
	a1 = decode_int (arg1); // arg1 is length
	// arg2 is fill-object
	
	arg1 = alloc_ram_cell_init (COMPOSITE_FIELD0 | (a1 >> 8),
	                            a1 & 0xff,
	                            U8VECTOR_FIELD2,
	                            0); // will be filled in later
	a3 = alloc_vec_cell (a1 << 1 /* each object field needs 2 bytes */, arg1);
	ram_set_cdr(arg1, a3);
	arg3 = _SYS_RAM_TO_VEC_OBJ(a3);
	// TODO: very janky, bypasses helper function, do not like:
	IF_TRACE((debug_printf("Writing %zu of ", (size_t)a1), show_obj(a2), debug_printf(" to %hu (vec cell %hu)\n", arg3, a3)));
	{
		const uint8 upper = arg2 >> 8;
		const uint8 lower = arg2 & 0xff;
		for(i = 0; i < a1; ++i) {
			// we don't care which is cons and which is cdr
			// because external API does not involve pairs
			// but we do need to have pair layout for the GC
			// and we want to otherwise preserve the ordering
			// of vector indices
			if (i & 0x01) {
#ifdef CONFIG_LITTLE_ENDIAN
				ram_set_field1(arg3, lower);
				ram_set_field0(arg3, COMPOSITE_FIELD0 | upper);
#else
				ram_set_field2(arg3, PAIR_FIELD2 | upper);
				ram_set_field3(arg3, lower);
#endif
				arg3 += 1;
			} else {
#ifdef CONFIG_LITTLE_ENDIAN
				ram_set_field3(arg3, lower);
				ram_set_field2(arg3, PAIR_FIELD2 | upper);
#else
				ram_set_field0(arg3, COMPOSITE_FIELD0 | upper);
				ram_set_field1(arg3, lower);
#endif
			}
		}
	}
	// if we have a dangling field, set it to #f
	// for GC safety
	if (i & 0x01) {
#ifdef CONFIG_LITTLE_ENDIAN
		ram_set_field1(arg3, OBJ_FALSE & 0xFF);
		ram_set_field0(arg3, COMPOSITE_FIELD0 | (OBJ_FALSE >> 8));
#else
		ram_set_field2(arg3, PAIR_FIELD2 | (OBJ_FALSE >> 8));
		ram_set_field3(arg3, OBJ_FALSE & 0xFF);
#endif
	}
	arg2 = OBJ_FALSE;
	arg3 = OBJ_FALSE;
}

PRIMITIVE(vector-ref, vector_ref, 2)
{
	a2 = decode_int (arg2);
	arg2 = OBJ_FALSE;

	// TODO adapt for the new bignums
	if (IN_RAM(arg1)) {
		if (!RAM_VECTOR_P(arg1)) {
			TYPE_ERROR("vector-ref.0", "vector");
		}

		if (ram_get_car (arg1) <= a2) {
			ERROR("vector-ref.0", "vector index invalid");
		}
		arg1 = ram_get_cdr (arg1);
		arg1 = _SYS_VEC_TO_RAM_OBJ(arg1) + (a2 >> 1);
#ifdef CONFIG_LITTLE_ENDIAN
		if (a2 & 0x0001) {
			arg1 = ram_get_car(arg1);
		} else {
			arg1 = ram_get_cdr(arg1);
		}
#else
		if (a2 & 0x0001) {
			arg1 = ram_get_cdr(arg1);
		} else {
			arg1 = ram_get_car(arg1);
		}
#endif
	} else if (IN_ROM(arg1)) {
		if (!ROM_VECTOR_P(arg1)) {
			TYPE_ERROR("vector-ref.1", "vector");
		}

		if (rom_get_car (arg1) <= a2) {
			ERROR("vector-ref.1", "vector index invalid");
		}
		arg1 = rom_get_cdr (arg1);
		arg1 = _SYS_VEC_TO_ROM_OBJ(arg1) + (a2 >> 1);
#ifdef CONFIG_LITTLE_ENDIAN
		if (a2 & 0x0001) {
			arg1 = rom_get_car(arg1);
		} else {
			arg1 = rom_get_cdr(arg1);
		}
#else
		if (a2 & 0x0001) {
			arg1 = rom_get_cdr(arg1);
		} else {
			arg1 = rom_get_car(arg1);
		}
#endif
	} else {
		TYPE_ERROR("vector-ref.2", "vector");
	}
}

PRIMITIVE_UNSPEC(vector-set!, vector_set, 3)
	// TODO a lot in common with ref, abstract that
{
	a2 = decode_int (arg2); // TODO adapt for bignums

	if (IN_RAM(arg1)) {
		if (!RAM_VECTOR_P(arg1)) {
			TYPE_ERROR("vector-set!.0", "vector");
		}

		if (ram_get_car (arg1) <= a2) {
			ERROR("vector-set!", "vector index invalid");
		}
		
		arg1 = _SYS_VEC_TO_RAM_OBJ(arg1) + (a2 >> 1);
#ifdef CONFIG_LITTLE_ENDIAN
		if (a2 & 0x0001) {
			ram_set_car(arg1, arg3);
		} else {
			ram_set_cdr(arg1, arg3);
		}
#else
		if (a2 & 0x0001) {
			ram_set_cdr(arg1, arg3);
		} else {
			ram_set_car(arg1, arg3);
		}
#endif
	} else {
		TYPE_ERROR("vector-set!.2", "vector");
	}

	arg1 = OBJ_FALSE;
	arg2 = OBJ_FALSE;
	arg3 = OBJ_FALSE;
}

PRIMITIVE(vector-length, vector_length, 1)
{
	if (IN_RAM(arg1)) {
		if (!RAM_VECTOR_P(arg1)) {
			TYPE_ERROR("vector-length.0", "vector");
		}

		arg1 = encode_int (ram_get_car (arg1));
	} else if (IN_ROM(arg1)) {
		if (!ROM_VECTOR_P(arg1)) {
			TYPE_ERROR("vector-length.1", "vector");
		}

		arg1 = encode_int (rom_get_car (arg1));
	} else {
		TYPE_ERROR("vector-length.2", "vector");
	}
}
