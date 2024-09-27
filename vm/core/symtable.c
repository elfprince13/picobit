#include <symtable.h>
#include <bignum.h>
#include <gc.h>
#include <primitives.h>
#include <debug.h>

uint16 symTableSize = 0;
uint16 symTableBuckets = 0;
uint16 symTableBucketMask = 0xffff;

extern void prim_make_vector();
extern void prim_vector_set();
extern void prim_vector_ref();
extern void prim_string_compare();

uint16 hash_string_buffer(obj str) {
	uint16 end;
	uint16 address;
	uint8 inRam = 0;
	if (IN_RAM(str) && RAM_STRING_P(str)) {
		inRam = 1;
		address = VEC_TO_RAM_BASE_ADDR(ram_get_cdr(str));
		end = address + ram_get_car(str);
	} else if (IN_ROM(str) && ROM_STRING_P(str)) {
		address = VEC_TO_ROM_BASE_ADDR(rom_get_cdr(str));
		end = address + rom_get_car(str);
	} else {
		/*
		uint8 typeTag;
		if (str < MIN_ROM_ENCODING) {
			typeTag = 0;
			debug_printf("Got unboxed value %04hx\n", str);
		} else if (str < MIN_RAM_ENCODING) {
			typeTag = ROM_EXTRACT_TYPE_TAG(str);
		} else if (str <= MAX_RAM_ENCODING) {
			typeTag = RAM_EXTRACT_TYPE_TAG(str);
		} else {
			typeTag = 0;
			debug_printf("in vector space? %04hx\n", str);
		}
		debug_printf("Type tag %02hhx, expected string (%02hhx)\n", typeTag, (uint8)String);
		*/
		TYPE_ERROR("hash_string_buffer", "string");
	}
	uint16 hash = 40507; // magic salt: prime AND close to 65536/phi for maximum superstition
	if (inRam) {
		for (; address != end; ++address) {
			hash = uhash_combine(hash, ram_get(address));
		}
	} else {
		for (; address != end; ++address) {
			hash = uhash_combine(hash, rom_get(address));
		}
	}
	return hash;
}

// this method assumes that sym is a symbol and that str is the reference
// obtained from its (car ...)
obj unsafe_intern_symbol_given_string(const obj sym, const obj str) {
    const uint16 truncHash = hash_string_buffer(str) & symTableBucketMask;
    const obj bucketIndex = encode_int(truncHash);
    arg1 = symTable;
    arg2 = bucketIndex;
    prim_vector_ref();

    a2 = (a1 = arg1);
    while (a2 != OBJ_NULL) {
        const obj headSym = ram_get_car(a2);
        const obj headStr = IN_RAM(headSym) ? ram_get_car(headSym) : rom_get_car(headSym);
        arg1 = str;
        arg2 = headStr;
        prim_string_compare();
        int cmpResult = (int16_t)decode_int(arg1);
        if (cmpResult < 0) {
            a1 = a2;
            a2 = ram_get_cdr(a2);
        } else if (cmpResult > 0) update_bucket_root: {
            const obj newCell = cons(sym, a2);
            symTableSize += 1;
            if (a1 == a2) {
                arg1 = symTable;
                arg2 = bucketIndex;
                arg3 = newCell;
                prim_vector_set();
            } else {
                ram_set_cdr(a1, newCell);
            }
            return sym;
        } else {
            return headSym;
        }
    }
    goto update_bucket_root;
}

obj intern_symbol(const obj sym) {
    if (IN_RAM(sym) && RAM_SYMBOL_P(sym)) {
        return unsafe_intern_symbol_given_string(sym, ram_get_car(sym));
    } else if (IN_ROM(sym) && ROM_SYMBOL_P(sym)) {
        return unsafe_intern_symbol_given_string(sym, rom_get_car(sym));
    } else {
        TYPE_ERROR("<intern_symbol>", "symbol");
    }
}

void init_sym_table(const uint8 numConstants)
{

	const obj end = MIN_ROM_ENCODING + numConstants;
    uint8 numSymbols = 0;
	for (obj romObj = MIN_ROM_ENCODING; romObj != end; ++romObj) {
        const PicobitType typeTag = (PicobitType)(ROM_EXTRACT_TYPE_TAG(romObj));

        IF_TRACE(debug_printf("Checking rom obj %hu / %hu: has type tag %02hhx (scanning for symbol %02hhx)\n",
					romObj - MIN_ROM_ENCODING + 1, numConstants,
					typeTag, (uint8)Symbol));

        numSymbols += (Symbol == typeTag);
    }
	// load factor will randomly be between 0.5 and 1
	symTableBuckets = npot(numSymbols);
    symTableBucketMask = symTableBuckets - (uint16)1;
    symTableSize = 0;
	
	arg1 = encode_int(symTableBuckets);
	arg2 = OBJ_NULL;
	prim_make_vector();
	symTable = arg1;

	for (obj romObj = MIN_ROM_ENCODING; romObj != end; ++romObj) {
		const PicobitType typeTag = (PicobitType)(ROM_EXTRACT_TYPE_TAG(romObj));
		
		if (Symbol == typeTag) {
			if (romObj != unsafe_intern_symbol_given_string(romObj, rom_get_car(romObj))) {
                ERROR("<init_sym_table>", "duplicate constant symbol - compiler bug?");
            }
            IF_TRACE((show_obj(symTable), debug_printf("\n")));
		}
	}
}