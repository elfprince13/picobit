#include <picobit.h>
#include <primitives.h>
#include <bignum.h>
#include <debug.h>
#include <escapes.h>

#include <stdio.h>
#include <time.h>

#ifdef CONFIG_ARCH_EZ80

void show (obj o)
{
#if 0
	printf ("[%d]", o);
#endif

	if (o == OBJ_FALSE) {
		printf ("#f");
	} else if (o == OBJ_TRUE) {
		printf ("#t");
	} else if (o == OBJ_NULL) {
		printf ("()");
	} else if (o <= (MIN_FIXNUM_ENCODING + (MAX_FIXNUM - MIN_FIXNUM))) {
		printf ("%d", DECODE_FIXNUM(o));
	} else {
		uint8 in_ram;

		if (IN_RAM(o)) {
			in_ram = 1;
		} else {
			in_ram = 0;
		}

		if ((in_ram && RAM_BIGNUM_P(o)) || (!in_ram && ROM_BIGNUM_P(o))) { // TODO fix for new bignums, especially for the sign, a -5 is displayed as 251
			printf ("%d", decode_int (o));
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

				printf ("(");

loop:

				show (car);

				if (cdr == OBJ_NULL) {
					printf (")");
				} else if ((IN_RAM(cdr) && RAM_PAIR_P(cdr))
				           || (IN_ROM(cdr) && ROM_PAIR_P(cdr))) {
					if (IN_RAM(cdr)) {
						car = ram_get_car (cdr);
						cdr = ram_get_cdr (cdr);
					} else {
						car = rom_get_car (cdr);
						cdr = rom_get_cdr (cdr);
					}

					printf (" ");
					goto loop;
				} else {
					printf (" . ");
					show (cdr);
					printf (")");
				}
			} else if ((in_ram && RAM_SYMBOL_P(o)) || (!in_ram && ROM_SYMBOL_P(o))) {
				printf ("#<symbol>");
			} else if ((in_ram && RAM_STRING_P(o)) || (!in_ram && ROM_STRING_P(o))) {
				const uint8 * charIt = (in_ram 
				  ? (ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(o))) 
				  : (rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(o))));
				putchar('"');
				for (const uint8 * const end = charIt + (in_ram ? ram_get_car(o) : rom_get_car(o));
				     charIt != end;
					 ++charIt)
				{
					uint8 c = *charIt;
					uint8* escaped;
					if (0 == schemeEscapeChar(c, &escaped)) {
						while((c = *(escaped++))) {
							putchar(c);
						}
					} else {
						putchar(c);
					}
				}
				putchar('"');
			} else if ((in_ram && RAM_U8VECTOR_P(o)) || (!in_ram && ROM_U8VECTOR_P(o))) {
				const uint8 * numIt = (in_ram 
				  ? (ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(o))) 
				  : (rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(o))));
				printf("'#u8(");
				const uint8 * const end = numIt + (in_ram ? ram_get_car(o) : rom_get_car(o));
				if (numIt != end) {
				u8vec_show_loop:
					printf("%hhu", *numIt);
					++numIt;
					if (numIt != end) {
						putchar(' ');
						/* annoyingly tricky to write this loop with a standard construct without repeating tests */
						goto u8vec_show_loop;
					}
				}
				printf(")");
			} else if ((in_ram && RAM_VECTOR_P(o)) || (!in_ram && ROM_VECTOR_P(o))) {
				const uint16 * numIt = (const uint16*)(in_ram 
				  ? (ram_mem + VEC_TO_RAM_BASE_ADDR(ram_get_cdr(o))) 
				  : (rom_mem + VEC_TO_ROM_BASE_ADDR(rom_get_cdr(o))));
				printf("'#(");
				const uint16 * const end = numIt + (in_ram ? ram_get_car(o) : rom_get_car(o));
				if (numIt != end) {
				vec_show_loop:
					show(*numIt);
					++numIt;
					if (numIt != end) {
						putchar(' ');
						/* annoyingly tricky to write this loop with a standard construct without repeating tests */
						goto vec_show_loop;
					}
				}
				printf(")");
			} else {
				printf ("(");
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

			printf ("{0x%04x ", pc);
			show (env);
			printf ("}");
		}
	}

	fflush (stdout);
}

void print (obj o)
{
	show (o);
	printf ("\n");
	fflush (stdout);
}

#endif

PRIMITIVE_UNSPEC(print, print, 1)
{
#ifdef CONFIG_ARCH_EZ80
	print (arg1);
#endif

	arg1 = OBJ_FALSE;
}


uint32 read_clock ()
{
	uint32_t now = 0;


#ifdef CONFIG_ARCH_EZ80
	static uint32_t start = 0;

    now = (1000 * clock()) / CLOCKS_PER_SEC;

    if (start == 0) {
        start = now;
    }

    now -= start;
#endif

	return now;
}

PRIMITIVE(clock, clock, 0)
{
	arg1 = encode_int (read_clock ());
}

PRIMITIVE(#%getchar, getchar, 0)
{
	arg1 = encode_int (getchar ());
}

PRIMITIVE(#%putchar, putchar, 1)
{
	a1 = decode_int(arg1);
	if (a1 > 255) {
		ERROR("putchar", "argument out of range");
	}
	putchar (a1);
	fflush (stdout);

	arg1 = OBJ_FALSE;
}