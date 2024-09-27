#ifndef PICOBIT_GC_H
#define PICOBIT_GC_H
#ifdef __cplusplus
extern "C" {
#endif
/* TODO explain what each tag means, with 1-2 mark bits.
   Currently, they're described in IFL paper. */
#define GC_TAG_0_LEFT   (1<<5)
#define GC_TAG_1_LEFT   (2<<5)
#define GC_TAG_UNMARKED (0<<5)


#ifdef LESS_MACROS
inline uint8 RAM_EXTRACT_TYPE_TAG(uint16 o) {
	_Static_assert((uint8_t)((int8_t)0x80 >> 6) == 0xfe, "Platform is not performing sign extension on arithemtic shift?!?");
	const uint8 f0 = ram_get_field0(o);
	const uint8 f2 = ram_get_field2(o);
	// requires sign extension!
	const uint8 signFill = (uint8_t)(((int8_t)f0) >> 6) /* possible values 0xfe, 0xff, 0x00, 0x01 */;

	return (((f2 & signFill) >> 5) | (signFill & ~(signFill >> 1)) | ((signFill >> 1) << 3)) & 0x0f;
}
inline uint8 ROM_EXTRACT_TYPE_TAG(uint16 o) {
	_Static_assert((uint8_t)((int8_t)0x80 >> 6) == 0xfe, "Platform is not performing sign extension on arithemtic shift?!?");
	const uint8 f0 = rom_get_field0(o);
	const uint8 f2 = rom_get_field2(o);
	// requires sign extension!
	const uint8 signFill = (uint8_t)(((int8_t)f0) >> 6) /* possible values 0xfe, 0xff, 0x00, 0x01 */;

	return (((f2 & signFill) >> 5) | (signFill & ~(signFill >> 1)) | ((signFill >> 1) << 3)) & 0x0f;
}
#else
#define _ETT_F0(aspace, o) (aspace ##_get_field0 (o))
#define _ETT_F2(aspace, o) (aspace ##_get_field2 (o))
#define _ETT_SF(f0) ((uint8_t)(((int8_t)f0) >> 6))
#define _ETT_RHS(f0,f2) ((((uint8_t)f2) & _ETT_SF(f0)) >> 5)
#define _ETT_LHS_C0(f0,f2) (_ETT_SF(f0) & ~(_ETT_SF(f0) >> 1))
#define _ETT_LHS_C1(f0,f2) ((_ETT_SF(f0) >> 1) << 3)
#define _ETT(f0, f2) ((_ETT_RHS(f0, f2) | _ETT_LHS_C0(f0,f2) | _ETT_LHS_C1(f0,f2)) & 0x0f)
#define RAM_EXTRACT_TYPE_TAG(o) _ETT(_ETT_F0(ram, o), _ETT_F2(ram, o))
#define ROM_EXTRACT_TYPE_TAG(o) _ETT(_ETT_F0(rom, o), _ETT_F2(rom, o))
#endif

typedef enum PicobitType {
	Integer = 0x00,
	Closure = 0x01,
	/* 02 -> 07 are unrepresentable due to object encodings */
	Pair = 0x08,
	Symbol = 0x09,
	String = 0x0A,
	U8Vec = 0x0B,
	Continuation = 0x0C,
	Vector = 0x0D
} PicobitType;

/* Number of object fields of objects in ram */
#ifdef LESS_MACROS
uint8 HAS_N_OBJECT_FIELDS(PicobitType typeTag)
{
	return (typeTag == Vector); //(RAM_VECTOR_P(visit));
}
uint8 HAS_2_OBJECT_FIELDS(PicobitType typeTag)
{
	return (typeTag == Pair) || (typeTag == Continuation); //(RAM_PAIR_P(visit) || RAM_CONTINUATION_P(visit));
}

#ifdef CONFIG_BIGNUM_LONG
uint8 HAS_1_OBJECT_FIELD(PicobitType typeTag)
{
	return (typeTag == Symbol) || (typeTag == Closure) || (typeTag == Integer); //(RAM_SYMBOL_P(visit) || RAM_CLOSURE_P(visit) || RAM_BIGNUM_P(visit));
}
#else
uint8 HAS_1_OBJECT_FIELD(uint16 visit)
{
	return (typeTag == Symbol) || (typeTag == Closure); //(RAM_COMPOSITE_P(visit) || RAM_CLOSURE_P(visit));
}
#endif

#else
#define HAS_N_OBJECT_FIELDS(typeTag) (typeTag == Vector) //(RAM_VECTOR_P(visit))
#define HAS_2_OBJECT_FIELDS(typeTag) ((typeTag == Pair) || (typeTag == Continuation)) //(RAM_PAIR_P(visit) || RAM_CONTINUATION_P(visit))
#ifdef CONFIG_BIGNUM_LONG
#define HAS_1_OBJECT_FIELD(typeTag)  ((typeTag == Symbol) || (typeTag == Closure) || (typeTag == Integer)) //(RAM_SYMBOL_P(visit) || RAM_CLOSURE_P(visit) || RAM_BIGNUM_P(visit))
#else
#define HAS_1_OBJECT_FIELD(typeTag)  ((typeTag == Symbol) || (typeTag == Closure)) //(RAM_SYMBOL_P(visit) || RAM_CLOSURE_P(visit))
#endif
#endif
// all composites except pairs and continuations have 1 object field
// however only symbols have 1 object field in the car position
// strings, u8vectors, and vectors have a vector reference in the cdr position

#ifdef CONFIG_GC_DEBUG
extern int max_live;
#endif

void init_ram_heap ();

void mark (obj temp);
void sweep ();
void gc ();

obj alloc_ram_cell ();
obj alloc_ram_cell_init (uint8 f0, uint8 f1, uint8 f2, uint8 f3);
obj alloc_vec_cell (uint16 n);
#ifdef __cplusplus
}
#endif
#endif
