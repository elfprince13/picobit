#include <symtable.h>
#include <bignum.h>

extern void prim_make_vector();
extern void prim_vector_set();
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

void init_sym_table(uint8 numConstants)
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
	const uint16 numBuckets = npot(numConstants);
	
	arg1 = encode_int(numBuckets);
	arg2 = OBJ_NULL;
	prim_make_vector();
	symTable = arg1;

	for (obj romObj = MIN_ROM_ENCODING; romObj != end; ++romObj) {
		/*
		const uint8 typeTag = ROM_EXTRACT_TYPE_TAG(romObj);
		debug_printf("Checking rom obj %hu / %hu: has type tag %02hhx (scanning for symbol %02hhx)\n",
					romObj - MIN_ROM_ENCODING + 1, numConstants,
					typeTag, (uint8)Symbol);
		*/
		if (ROM_SYMBOL_P(romObj)) {
			// hash and insert
			arg1 = symTable;
			arg2 = encode_int(hash_string_buffer(rom_get_car(romObj)) & ~((uint16_t)1 + ~numBuckets));
			arg3 = romObj;
			prim_vector_set();
		}
	}
}