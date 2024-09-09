#include <picobit.h>
#include <bignum.h>
#include <debug.h>
#include <stdio.h>
#include <dispatch.h>
#include <escapes.h>

void show_type (obj o)
{
	debug_printf("%04x : ", o);

	if (o == OBJ_FALSE) {
		debug_printf("#f");
	} else if (o == OBJ_TRUE) {
		debug_printf("#t");
	} else if (o == OBJ_NULL) {
		debug_printf("()");
	} else if (o < MIN_ROM_ENCODING) {
		debug_printf("fixnum");
	} else if (IN_RAM (o)) {
		if (o > MAX_RAM_ENCODING) {
			debug_printf("ram vector cell");
		} else if (RAM_BIGNUM_P(o)) {
			debug_printf("ram bignum");
		} else if (RAM_PAIR_P(o)) {
			debug_printf("ram pair");
		} else if (RAM_SYMBOL_P(o)) {
			debug_printf("ram symbol");
		} else if (RAM_STRING_P(o)) {
			debug_printf("ram string");
		} else if (RAM_U8VECTOR_P(o)) {
			debug_printf("ram u8vector");
		} else if (RAM_CONTINUATION_P(o)) {
			debug_printf("ram continuation");
		} else if (RAM_CLOSURE_P(o)) {
			debug_printf("ram closure");
		} else if (RAM_VECTOR_P(o)) {
			debug_printf("ram vector");
		} else {
			debug_printf("ram unknown");
		}
	} else { // ROM
		if (ROM_BIGNUM_P(o)) {
			debug_printf("rom bignum");
		} else if (ROM_PAIR_P(o)) {
			debug_printf("rom pair");
		} else if (ROM_SYMBOL_P(o)) {
			debug_printf("rom symbol");
		} else if (ROM_STRING_P(o)) {
			debug_printf("rom string");
		} else if (ROM_U8VECTOR_P(o)) {
			debug_printf("rom u8vector");
		} else if (ROM_CONTINUATION_P(o)) {
			debug_printf("rom continuation");
		} else if (ROM_VECTOR_P(o)) {
			debug_printf("rom vector");
		} else {
			debug_printf("rom unknown");
		}

		// ROM closures don't exist
	}

	//debug_printf("\n");
}

void show_obj (obj o)
{
	// TODO: not thread safe, but this whole method needs reworking!
	static int recursionDepth = -1;
	++recursionDepth;


	if (o == OBJ_FALSE) {
		debug_printf ("#f");
	} else if (o == OBJ_TRUE) {
		debug_printf ("#t");
	} else if (o == OBJ_NULL) {
		debug_printf ("()");
	} else if (o <= (MIN_FIXNUM_ENCODING + (MAX_FIXNUM - MIN_FIXNUM))) {
		debug_printf ("%d", DECODE_FIXNUM(o));
	} else {
		uint8 in_ram;

		if (IN_RAM(o)) {
			in_ram = 1;
		} else {
			in_ram = 0;
		}

		uint16 loopCount = 0;

		if ((in_ram && RAM_BIGNUM_P(o)) || (!in_ram && ROM_BIGNUM_P(o))) { // TODO fix for new bignums, especially for the sign, a -5 is displayed as 251
			debug_printf ("%d", decode_int (o));
		} else if ((in_ram && RAM_COMPOSITE_P(o)) || (!in_ram && ROM_COMPOSITE_P(o))) {
			obj car;
			obj cdr;

			if ((in_ram && RAM_PAIR_P(o)) || (!in_ram && ROM_PAIR_P(o))) {
				if (in_ram) {
					car = ram_get_car (o);
					cdr = ram_get_cdr (o);
				} else {
					car = rom_get_car (o);
					cdr = rom_get_cdr (o);
				}

				debug_printf ("(");
loop:
				if ((loopCount > 32) || (recursionDepth > 32)) {
					debug_printf("...debug recursion limit exceeded...)");
				} else {
					show_obj (car);
					if (cdr == OBJ_NULL) {
						debug_printf (")");
					} else if ((IN_RAM(cdr) && RAM_PAIR_P(cdr))
						|| (IN_ROM(cdr) && ROM_PAIR_P(cdr))) {
						if (IN_RAM(cdr)) {
							car = ram_get_car (cdr);
							cdr = ram_get_cdr (cdr);
						} else {
							car = rom_get_car (cdr);
							cdr = rom_get_cdr (cdr);
						}

						debug_printf (" ");
						++loopCount;
						goto loop;
					} else {
						debug_printf (" . ");
						show_obj (cdr);
						debug_printf (")");
					}
				}
			} else if ((in_ram && RAM_SYMBOL_P(o)) || (!in_ram && ROM_SYMBOL_P(o))) {
				o = in_ram ? ram_get_car(o) : rom_get_car(o);
				in_ram = IN_RAM(o);
				debug_printf ("'%s", (in_ram ? (ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(o))) : (rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(o)))));
			} else if ((in_ram && RAM_STRING_P(o)) || (!in_ram && ROM_STRING_P(o))) {
				const uint8 * charIt = (in_ram 
				  ? (ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(o))) 
				  : (rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(o))));
				debug_printf("\"");
				for (const uint8 * const end = charIt + (in_ram ? ram_get_car(o) : rom_get_car(o));
				     charIt != end;
					 ++charIt)
				{
					uint8 c = *charIt;
					uint8* escaped;
					if (0 == schemeEscapeChar(c, &escaped)) {
						debug_printf("%s", escaped);
					} else {
						debug_printf("%c", c);
					}
				}
				debug_printf("\"");
			} else if ((in_ram && RAM_U8VECTOR_P(o)) || (!in_ram && ROM_U8VECTOR_P(o))) {
				
				const uint8 * numIt = (in_ram 
				  ? (ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(o))) 
				  : (rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(o))));
				debug_printf("'#u8(");
				const uint8 * const end = numIt + (in_ram ? ram_get_car(o) : rom_get_car(o));
				if (numIt != end) {
				u8vec_show_loop:
					debug_printf("%hhu", *numIt);
					++numIt;
					if (numIt != end) {
						debug_printf(" ");
						/* annoyingly tricky to write this loop with a standard construct without repeating tests */
						goto u8vec_show_loop;
					}
				}
				debug_printf(")");
			} else if ((in_ram && RAM_VECTOR_P(o)) || (!in_ram && ROM_VECTOR_P(o))) {
				const uint16 * numIt = (const uint16*)(in_ram 
				  ? (ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(o))) 
				  : (rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(o))));
				debug_printf("'#(");
				const uint16 * const end = numIt + (in_ram ? ram_get_car(o) : rom_get_car(o));
				if (numIt != end) {
				vec_show_loop:
					show_obj((*numIt) & 0x1fff);
					++numIt;
					if (numIt != end) {
						debug_printf(" ");
						/* annoyingly tricky to write this loop with a standard construct without repeating tests */
						goto vec_show_loop;
					}
				}
				debug_printf(")");
			} else {
				debug_printf ("(");
				cdr = ram_get_car (o);
				car = ram_get_cdr (o);
				// ugly hack, takes advantage of the fact that pairs and
				// continuations have the same layout
				goto loop;
			}
		} else { // closure
			obj env;
			rom_addr pc;

			env = ram_get_car (o);
			pc = ram_get_entry (o);

			debug_printf ("{0x%04x ", pc);
			show_obj (env);
			debug_printf ("}");
		}
	}

	--recursionDepth;
	fflush (stdout);
}

void show_state (rom_addr pc) {
	uint8 byteCode = rom_get (pc);
	debug_printf ("\n");
	debug_printf ("pc=0x%04x bytecode=0x", pc);
	if (PUSH_CONSTANT_LONG == (byteCode >> 4)) {
		debug_printf("%01x_ ", byteCode >> 4);
	} else {
		debug_printf("%02x", byteCode);
	}
	debug_printf(" env=");
	show_obj (env);
	debug_printf (" cont=");
	show_obj (cont);
	debug_printf ("\n");
	fflush (stdout);
}

const char* _show_obj_bytes (obj o, char buffer[]) {
	if(IN_RAM(o)) {
		snprintf(buffer, 32, "%s:%04hx = ", ((o > MAX_RAM_ENCODING) ? "VEC" : "RAM"), o);
		if (OBJ_TO_RAM_ADDR(o, 0) <= (VEC_BYTES + RAM_BYTES) - 4) {
			snprintf(buffer + 11, 21, "%02hx,%02hx,%02hx,%02hx",
			             ram_get_field0(o), ram_get_field1(o), ram_get_field2(o), ram_get_field3(o));
		} else {
			snprintf(buffer + 11, 21, "OOB!");
		}
		
	} else {
		snprintf(buffer, 32, "ROM:%04hx = ", o);
		if (OBJ_TO_ROM_ADDR(o, 0) <= ROM_BYTES - 4) {
			snprintf(buffer + 11, 21, "%02hx,%02hx,%02hx,%02hx",
			             rom_get_field0(o), rom_get_field1(o), rom_get_field2(o), rom_get_field3(o));
		} else {
			snprintf(buffer + 11, 21, "OOB!");
		}
	}
	return buffer;
}

void show_obj_bytes (obj o) {
	static char buffer[32];
	debug_printf("%s",_show_obj_bytes(o, buffer));
}
