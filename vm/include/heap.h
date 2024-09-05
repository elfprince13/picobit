#ifndef PICOBIT_HEAP_H
#define PICOBIT_HEAP_H
/*
 * Address space layout.
 * For details, see IFL paper. Pointer in README.
 *
 * Vector space is in RAM too, but separate from the regular heap
 * (address spaces are disjoint).
 * It can reuse helper functions (ram_get_car, etc.) defined for the
 * regular heap.
 * On the target device, vector space should be right after the
 * regular heap.
 *
 * Boundaries between zones can be changed to better fit a target
 * sytems's or an application's needs.
 * Some invariants must be respected:
 *  - the order of the zones must not change
 *  - these constants must be kept in sync with the compiler's
 *    (in encoding.rkt)
 *  - -1 and 0 must be fixnums, otherwise bignums won't work
 *  - vector space can overlap with ram, rom, and constant encodings
 *    but all other zones must be distinct
 *  - the largest encoding is bounded by the pointer size in the
 *    object layout
 */

#include <arch/memory.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_VEC_ENCODING 8191
#define MIN_VEC_ENCODING 0
#define VEC_BYTES ((MAX_VEC_ENCODING - MIN_VEC_ENCODING + 1)*4)

#define MAX_RAM_ENCODING 8191
#define MIN_RAM_ENCODING 1280
#define RAM_BYTES ((MAX_RAM_ENCODING - MIN_RAM_ENCODING + 1)*4)

#define MIN_FIXNUM_ENCODING 3
#define MIN_FIXNUM (-1)
#define MAX_FIXNUM 256
#define MIN_ROM_ENCODING (MIN_FIXNUM_ENCODING + MAX_FIXNUM - MIN_FIXNUM + 1)

#define ZERO ENCODE_FIXNUM(0)
#define NEG1 (ZERO-1)
#define POS1 (ZERO+1)

#ifdef CONFIG_LITTLE_ENDIAN
#define FIELD0_OFFSET 3
#define FIELD1_OFFSET 2
#define FIELD2_OFFSET 1
#define FIELD3_OFFSET 0
#else
#define FIELD0_OFFSET 0
#define FIELD1_OFFSET 1
#define FIELD2_OFFSET 2
#define FIELD3_OFFSET 3
#endif

#ifdef LESS_MACROS
uint16 OBJ_TO_RAM_ADDR(uint16 o, uint8 f)
{
	return ((((o) - MIN_RAM_ENCODING) << 2) + (f));
}
uint16 OBJ_TO_ROM_ADDR(uint16 o, uint8 f)
{
	return ((((o) - MIN_ROM_ENCODING) << 2) + (CODE_START + 4 + (f)));
}
uint16 VEC_TO_RAM_BASE_ADDR(uint16 o)
{
	return (o + MAX_RAM_ENCODING + 1 - MIN_RAM_ENCODING) << 2;
}
uint16 VEC_TO_ROM_BASE_ADDR(uint16 o)
{
	return (o << 2) + (CODE_START + 4);
}
// danger! don't use these with OBJ_TO_RAM_ADDR to access individual bytes
// as the code is endian-agnostic.
uint16 _SYS_VEC_TO_RAM_OBJ(uint16 o)
{
	return o + MAX_RAM_ENCODING;
}
uint16 _SYS_RAM_TO_VEC_OBJ(uint16 o)
{
	return o - MAX_RAM_ENCODING;
}
uint16 _SYS_VEC_TO_ROM_OBJ(uint16 o)
{
	return o + MIN_ROM_ENCODING;
}
uint16 _SYS_ROM_TO_VEC_OBJ(uint16 o)
{
	return o - MIN_ROM_ENCODING;
}
#else
#define OBJ_TO_RAM_ADDR(o,f) (((uint16)((o) - MIN_RAM_ENCODING) << 2) + (f))
#define OBJ_TO_ROM_ADDR(o,f) (((uint16)((o) - MIN_ROM_ENCODING) << 2) + (CODE_START + 4 + (f)))
#define VEC_TO_RAM_BASE_ADDR(o) ((uint16)(((uint16)(o) + MAX_RAM_ENCODING + 1 - MIN_RAM_ENCODING) << 2))
#define VEC_TO_ROM_BASE_ADDR(o) ((uint16)(((uint16)(o) << 2) + (CODE_START + 4)))
// danger! don't use these with OBJ_TO_RAM_ADDR to access individual bytes
// as the code is endian-agnostic.
#define _SYS_VEC_TO_RAM_OBJ(o) ((o) + MAX_RAM_ENCODING + 1)
#define _SYS_RAM_TO_VEC_OBJ(o) ((o) - MAX_RAM_ENCODING - 1)
#define _SYS_VEC_TO_ROM_OBJ(o) ((o) + MIN_ROM_ENCODING)
#define _SYS_ROM_TO_VEC_OBJ(o) ((o) - MIN_ROM_ENCODING)
#endif

#ifdef LESS_MACROS
uint8 ram_get_field0(uint16 o)
{
	return ram_get (OBJ_TO_RAM_ADDR(o,FIELD0_OFFSET));
}
void  ram_set_field0(uint16 o, uint8 val)
{
	ram_set (OBJ_TO_RAM_ADDR(o,FIELD0_OFFSET), val);
}
uint8 rom_get_field0(uint16 o)
{
	return rom_get (OBJ_TO_ROM_ADDR(o,FIELD0_OFFSET));
}
#else
#define ram_get_field0(o) ram_get (OBJ_TO_RAM_ADDR(o,FIELD0_OFFSET))
#define ram_set_field0(o,val) ram_set (OBJ_TO_RAM_ADDR(o,FIELD0_OFFSET), val)
#define rom_get_field0(o) rom_get (OBJ_TO_ROM_ADDR(o,FIELD0_OFFSET))
#endif

#ifdef LESS_MACROS
uint8 ram_get_gc_tags(uint16 o)
{
	return (ram_get_field0(o) & 0x60);
}
uint8 ram_get_gc_tag0(uint16 o)
{
	return (ram_get_field0(o) & 0x20);
}
uint8 ram_get_gc_tag1(uint16 o)
{
	return (ram_get_field0(o) & 0x40);
}
void  ram_set_gc_tags(uint16 o, uint8 tags)
{
	ram_set_field0(o,(ram_get_field0(o) & 0x9f) | (tags));
}
void  ram_set_gc_tag0(uint16 o, uint8 tag)
{
	ram_set_field0(o,(ram_get_field0(o) & 0xdf) | (tag));
}
void  ram_set_gc_tag1(uint16 o, uint8 tag)
{
	ram_set_field0(o,(ram_get_field0(o) & 0xbf) | (tag));
}
#else
#define ram_get_gc_tags(o) (ram_get_field0(o) & 0x60)
#define ram_get_gc_tag0(o) (ram_get_field0(o) & 0x20)
#define ram_get_gc_tag1(o) (ram_get_field0(o) & 0x40)
#define ram_set_gc_tags(o,tags) \
  (ram_set_field0(o,(ram_get_field0(o) & 0x9f) | (tags)))
#define ram_set_gc_tag0(o,tag)  \
  ram_set_field0(o,(ram_get_field0(o) & 0xdf) | (tag))
#define ram_set_gc_tag1(o,tag)  \
  ram_set_field0(o,(ram_get_field0(o) & 0xbf) | (tag))
#endif

#ifdef LESS_MACROS
#error "Must update code paths to support little endian config flag"
uint8 ram_get_field1(uint16 o)
{
	return ram_get (OBJ_TO_RAM_ADDR(o,FIELD1_OFFSET));
}
uint8 ram_get_field2(uint16 o)
{
	return ram_get (OBJ_TO_RAM_ADDR(o,FIELD2_OFFSET));
}
uint8 ram_get_field3(uint16 o)
{
	return ram_get (OBJ_TO_RAM_ADDR(o,FIELD3_OFFSET));
}
void  ram_set_field1(uint16 o, uint8 val)
{
	ram_set (OBJ_TO_RAM_ADDR(o,FIELD1_OFFSET), val);
}
void  ram_set_field2(uint16 o, uint8 val)
{
	ram_set (OBJ_TO_RAM_ADDR(o,FIELD2_OFFSET), val);
}
void  ram_set_field3(uint16 o, uint8 val)
{
	ram_set (OBJ_TO_RAM_ADDR(o,FIELD3_OFFSET), val);
}
uint8 rom_get_field1(uint16 o)
{
	return rom_get (OBJ_TO_ROM_ADDR(o,FIELD1_OFFSET));
}
uint8 rom_get_field2(uint16 o)
{
	return rom_get (OBJ_TO_ROM_ADDR(o,FIELD2_OFFSET));
}
uint8 rom_get_field3(uint16 o)
{
	return rom_get (OBJ_TO_ROM_ADDR(o,FIELD3_OFFSET));
}
#else
#define ram_get_field1(o) ram_get (OBJ_TO_RAM_ADDR(o,FIELD1_OFFSET))
#define ram_get_field2(o) ram_get (OBJ_TO_RAM_ADDR(o,FIELD2_OFFSET))
#define ram_get_field3(o) ram_get (OBJ_TO_RAM_ADDR(o,FIELD3_OFFSET))
#define ram_set_field1(o,val) ram_set (OBJ_TO_RAM_ADDR(o,FIELD1_OFFSET), val)
#define ram_set_field2(o,val) ram_set (OBJ_TO_RAM_ADDR(o,FIELD2_OFFSET), val)
#define ram_set_field3(o,val) ram_set (OBJ_TO_RAM_ADDR(o,FIELD3_OFFSET), val)
#define rom_get_field1(o) rom_get (OBJ_TO_ROM_ADDR(o,FIELD1_OFFSET))
#define rom_get_field2(o) rom_get (OBJ_TO_ROM_ADDR(o,FIELD2_OFFSET))
#define rom_get_field3(o) rom_get (OBJ_TO_ROM_ADDR(o,FIELD3_OFFSET))
#endif

obj cons (obj car, obj cdr);

obj ram_get_car (obj o);
obj rom_get_car (obj o);
obj ram_get_cdr (obj o);
obj rom_get_cdr (obj o);
void ram_set_car (obj o, obj val);
void ram_set_cdr (obj o, obj val);

obj ram_get_entry (obj o);

obj get_global (uint8 i);
void set_global (uint8 i, obj o);
#ifdef __cplusplus
}
#endif
#endif
