#include <picobit.h>
#include <bignum.h>
#include <debug.h>
#include <gc.h>
#include <primitives.h>


static obj free_list, free_vec_pointer;

#ifdef CONFIG_GC_STATISTICS
int max_live = 0;
#endif

IF_GC_TRACE(extern const char* _show_obj_bytes(obj o, char buffer[]); char vBuf[32]; char sBuf[32]; char tBuf[32]);


void init_ram_heap ()
{
	uint8 i;
	obj o = MAX_RAM_ENCODING;
	uint16 bound = MIN_RAM_ENCODING + ((glovars + 1) >> 1);

	free_list = 0;

	while (o > bound) {
		// we don't want to add globals to the free list, and globals occupy the
		// beginning of memory at the rate of 2 globals per word (car and cdr)
		ram_set_gc_tags (o, GC_TAG_UNMARKED);
		ram_set_car (o, free_list);
		free_list = o;
		o--;
	}

	free_vec_pointer = _SYS_VEC_TO_RAM_OBJ(MIN_VEC_ENCODING);
	IF_GC_TRACE(debug_printf("free_vec_pointer initialized to %hu\n", free_vec_pointer));

	for (i = 0; i < glovars; i++) {
		set_global (i, OBJ_FALSE);
	}

	arg1 = OBJ_FALSE;
	arg2 = OBJ_FALSE;
	arg3 = OBJ_FALSE;
	arg4 = OBJ_FALSE;
	cont = OBJ_FALSE;
	env  = OBJ_NULL;

#ifdef CONFIG_BIGNUM_LONG
	bignum_gc_init();
#endif
}

_Static_assert(_ETT(0x3F, 0xFF) == 0x00, "number test failed");
_Static_assert(_ETT(0x7F, 0xFF) == 0x01, "closure test failed");
_Static_assert(_ETT(0xFF, 0x1F) == 0x08, "pair test failed");
_Static_assert(_ETT(0xFF, 0x3F) == 0x09, "symbol test failed");
_Static_assert(_ETT(0xFF, 0x5F) == 0x0A, "string test failed");
_Static_assert(_ETT(0xFF, 0x7F) == 0x0B, "u8vec test failed");
_Static_assert(_ETT(0xFF, 0x9F) == 0x0C, "cont. test failed");
_Static_assert(_ETT(0xFF, 0xBF) == 0x0D, "o-vec test failed");

void mark (obj temp)
{
	uint8 typeTag;
	/* mark phase */

	obj stack;
	obj visit;

	if (IN_RAM(temp)) {
		visit = OBJ_FALSE;

push:
		stack = visit;
		visit = temp;
post_push:
		typeTag = RAM_EXTRACT_TYPE_TAG(visit);
		IF_GC_TRACE((debug_printf ("push   stack=%d (%s) visit=%d (%s) (tag=%d)  (type=",
		                           stack, _show_obj_bytes(stack, sBuf), visit, _show_obj_bytes(visit, vBuf),
								   ram_get_gc_tags (visit)>>5), show_type(visit), debug_printf(" aka %02hhx)\n",typeTag)));
		
		if ((HAS_1_OBJECT_FIELD (typeTag) && ram_get_gc_tag0 (visit))
		    || ((HAS_2_OBJECT_FIELDS (typeTag) || HAS_N_OBJECT_FIELDS(typeTag))
		        && (ram_get_gc_tags (visit) != GC_TAG_UNMARKED))) {
			IF_GC_TRACE(debug_printf ("case 1\n"));
		} else {
			switch ((PicobitType)typeTag) {
				// 2 object fields
				case Pair: __attribute__ ((fallthrough));
				case Continuation:
					IF_GC_TRACE(debug_printf ("case 2\n"));

					temp = ram_get_cdr (visit);

					if (IN_RAM(temp)) {
						IF_GC_TRACE(debug_printf ("case 3:  stack=%d (%s) visit=%d (%s) temp=%d (%s)\n",
						stack, _show_obj_bytes(stack, sBuf),
						visit, _show_obj_bytes(visit, vBuf),
						temp, _show_obj_bytes(temp, tBuf)));
						ram_set_gc_tags (visit, GC_TAG_1_LEFT);
						ram_set_cdr (visit, stack);
						goto push;
					}

					IF_GC_TRACE(debug_printf ("case 4\n"));

					goto visit_field0;
				// 1 object field
				case Integer: __attribute__ ((fallthrough));
				case Closure: __attribute__ ((fallthrough));
				case Symbol:
					IF_GC_TRACE(debug_printf ("case 5\n"));

visit_field0:
					temp = ram_get_car (visit);

					if (IN_RAM(temp)) {
						IF_GC_TRACE(debug_printf ("case 6: %d\n",temp));
						ram_set_gc_tag0 (visit, GC_TAG_0_LEFT);
						ram_set_car (visit, stack);

						goto push;
					}

					IF_GC_TRACE(debug_printf ("case 7: %d\n",temp));
				// 1 field but can't point to any other objects
				case String: __attribute__ ((fallthrough));
				case U8Vec:
					IF_GC_TRACE(debug_printf ("case 8\n"));
					break;
				// Spooky: 1 field that is really N fields
				case Vector:
					// get the vector header
					temp = _SYS_VEC_TO_RAM_OBJ(ram_get_cdr(visit)) - 1;
#ifndef NDEBUG
					if (IN_RAM(temp)) { // this is probably unnecessary - remove when not debugging 
#endif //NDEBUG
						const uint16 blockCount = ram_get_car(temp);

						IF_GC_TRACE(do { 
							const uint16 length = ram_get_car(visit);
							if (blockCount != (1 + ((1 + length) >> 1))) {
								IF_GC_TRACE(debug_printf("Expected: %d, found %d\n", 1 + (length >> 1), blockCount));
								ERROR("gc.mark","broken obj vector header");
							}
						} while(0));

						if (blockCount > 1) { // don't visit the header itself, which is included in block count!
							// set WIP status on object
							ram_set_gc_tags(visit, GC_TAG_1_LEFT);
							const uint16 length = ram_get_car(visit);
							// replace back pointer with length as extra storage
							// we will need this to re-derive block count when
							// the finish traversing the vector
							ram_set_cdr(temp, length);
							// replace original length with stack
							ram_set_car(visit, stack);
							temp = temp + blockCount - 1;
							IF_GC_TRACE(debug_printf("case 9: stack=%d (%s) visit=%d (%s) temp=%d (%s)\n",
								stack, _show_obj_bytes(stack, sBuf),
								visit, _show_obj_bytes(visit, vBuf),
								temp, _show_obj_bytes(temp, tBuf)));
							stack = visit;
visit_fieldNodd:
							ram_set_gc_tags(temp, GC_TAG_1_LEFT);
							visit = ram_get_cdr(temp);
							if (IN_RAM(visit)) {
								IF_GC_TRACE(debug_printf("\tcase 9a: visit=%d (%s)\n",
									visit, _show_obj_bytes(visit, vBuf)));
								goto post_push;
							} else {
								IF_GC_TRACE(debug_printf("\tcase 9b\n"));
								goto pop_vector;
							}
						} else {
							IF_GC_TRACE(debug_printf("case 10\n"));
							// fall through to mark visited for real this time and pop...
						}
#ifndef NDEBUG
					} else {
						ERROR("gc.mark", "RAM vector pointing at ROM?");
					}
#endif // NDEBUG

			}

			ram_set_gc_tag0 (visit, GC_TAG_0_LEFT);
		}

pop:
		IF_GC_TRACE(debug_printf ("pop    stack=%d (%s)  visit=%d (%s) (tag=%d)\n",
		                          stack, _show_obj_bytes(stack, sBuf), visit, _show_obj_bytes(visit, vBuf),
								  ram_get_gc_tags (visit)>>6));

		if (stack != OBJ_FALSE) {
			typeTag = RAM_EXTRACT_TYPE_TAG(stack);
			switch((PicobitType)typeTag) {
				// 2 object fields
				case Pair: __attribute__ ((fallthrough));
				case Continuation:
					if (ram_get_gc_tag1(stack)) {
						IF_GC_TRACE(debug_printf ("case 11\n"));

						temp = ram_get_cdr (stack);  /* pop through cdr */
						ram_set_cdr (stack, visit);
						visit = stack;
						stack = temp;

						ram_set_gc_tag1(visit, GC_TAG_UNMARKED);
						// we unset the "1-left" bit

						goto visit_field0;
					}
					__attribute__ ((fallthrough));
				// 1 object field
				case Integer: __attribute__ ((fallthrough));
				case Closure: __attribute__ ((fallthrough));
				case Symbol:
					IF_GC_TRACE(debug_printf ("case 12\n"));

					temp = ram_get_car (stack);  /* pop through car */
					ram_set_car (stack, visit);
					visit = stack;
					stack = temp;

					goto pop;
				// Spooky: 1 field that is really N fields
				case Vector:
pop_vector:
					// get the vector header - don't need current visit anywhere,
					// we never overwrote its id in the vector
					visit = _SYS_VEC_TO_RAM_OBJ(ram_get_cdr(stack)) - 1;
					const uint16 blockCount = ram_get_car(visit);
					temp = visit + blockCount - 1;
					if (blockCount > 1) {
						IF_GC_TRACE(debug_printf("case 13: visit=%d (%s)\n", visit, _show_obj_bytes(visit, vBuf)));
						if (ram_get_gc_tag0(temp)) {
							IF_GC_TRACE(debug_printf("\tcase13a\n"));
							// done visiting here...
							ram_set_car(visit, blockCount - 1);
							if (--temp > visit) {
								IF_GC_TRACE(debug_printf("\t\tcase13a1\n"));
								goto visit_fieldNodd;
							} else {
								IF_GC_TRACE(debug_printf("\t\tcase13a2\n"));
								goto pop_vector;
							}
							
						} else if (ram_get_gc_tag1(temp)) {
							IF_GC_TRACE(debug_printf("\tcase13b\n"));
							ram_set_gc_tags(temp, GC_TAG_0_LEFT);
							visit = ram_get_car(temp);
							if (IN_RAM(visit)) {
								IF_GC_TRACE(debug_printf("\t\tcase13b1\n"));
								goto post_push;
							} else {
								IF_GC_TRACE(debug_printf("\t\tcase13b2\n"));
								goto pop_vector; 
							}
						} else {
							ERROR("gc.mark.pop", "inconsistent tag state");
						}
					} else if (ram_get_gc_tag1(stack)) {
						IF_GC_TRACE(debug_printf("case 14: stack=%d (%s) visit=%d (%s) temp=%d (%s)\n",
							stack, _show_obj_bytes(stack, sBuf),
							visit, _show_obj_bytes(visit, vBuf),
							temp, _show_obj_bytes(temp, tBuf)));
						// all done, set tag from in progress to done
						// and restore the field layout
						ram_set_gc_tags(stack, GC_TAG_0_LEFT);
						const uint16 length = ram_get_cdr(temp);
						ram_set_cdr(temp, stack);
						ram_set_car(temp, (1 + ((1 + length) >> 1)));
						temp = ram_get_car(stack);
						// if we set visit = length, it would be gross
						// but we could jump to case Closure: below
						// to save repeated code, at the cost of one
						// extra jump/pipeline flush
						ram_set_car(stack, length);
						visit = stack;
						stack = temp;
						goto pop;
					} else {
						ERROR("gc.mark.pop", "already visited");
					}
				// 1 field but can't point to any other objects
				case String: __attribute__ ((fallthrough));
				case U8Vec:
					ERROR("gc.mark.pop", "Unexpected type!");
			}
		}
	}
}

void sweep ()
{
	/* sweep phase */

#ifdef CONFIG_GC_STATISTICS
	int n = 0;
#endif

	obj visit = MAX_RAM_ENCODING;

	free_list = 0;

	while (visit >= (MIN_RAM_ENCODING + ((glovars + 1) >> 1))) {
		// we don't want to sweep the global variables area
		if ((RAM_COMPOSITE_P(visit)
		     && (ram_get_gc_tags (visit) == GC_TAG_UNMARKED)) // 2 mark bit
		    || (!RAM_COMPOSITE_P(visit)
		        && !(ram_get_gc_tags (visit) & GC_TAG_0_LEFT))) { // 1 mark bit
			/* unmarked? */
			if (RAM_U8VECTOR_P(visit) || RAM_STRING_P(visit) || RAM_VECTOR_P(visit)) {
				// when we sweep a vector, we also have to mark its contents as free
				// we subtract 1 to get to the header of the block, before the data
				obj o = _SYS_VEC_TO_RAM_OBJ(ram_get_cdr (visit) - 1);
				ram_set_gc_tag0 (o, 0); // mark the block as free
			}

			ram_set_car (visit, free_list);
			free_list = visit;
		} else {
			if (RAM_COMPOSITE_P(visit)) {
				ram_set_gc_tags (visit, GC_TAG_UNMARKED);
				if (RAM_VECTOR_P(visit)) {
					const obj start = _SYS_VEC_TO_RAM_OBJ(ram_get_cdr(visit));
					const obj header = start - 1;
					const uint16 blockCount = ram_get_car(header);
					IF_GC_TRACE(debug_printf("Vector %d (len %d, header at %d, blockcount %d) was marked\n", visit, ram_get_car(visit), header, blockCount));
					IF_GC_TRACE(do { 
						const uint16 length = ram_get_car(visit);
						if (blockCount != (1 + ((1 + length) >> 1))) {
							IF_GC_TRACE(debug_printf("Expected: %d, found %d\n", 1 + (length >> 1), blockCount));
							ERROR("gc.sweep","broken obj vector header");
						}
					} while(0));
					const obj end = start + blockCount - 1; // remove the header;
					for (obj subVisit = start; subVisit != end; ++subVisit) {
						IF_GC_TRACE(debug_printf("\tresetting mark on vector field %d, %s in [%d,%d)->", subVisit, _show_obj_bytes(subVisit, vBuf), start, end));
						ram_set_gc_tags(subVisit, GC_TAG_UNMARKED);
						IF_GC_TRACE(debug_printf("%s\n", _show_obj_bytes(subVisit, vBuf)));
					}
				}
			} else { // only 1 mark bit to unset
				ram_set_gc_tag0 (visit, GC_TAG_UNMARKED);
			}

#ifdef CONFIG_GC_STATISTICS
			n++;
#endif
		}

		visit--;
	}

#ifdef CONFIG_GC_STATISTICS
	if (n > max_live) {
		max_live = n;
#ifdef CONFIG_GC_DEBUG
		printf ("**************** memory needed = %d\n", max_live+1);
		fflush (stdout);
#endif
	}
#endif
}

void gc ()
{
	uint8 i;

	IF_TRACE(debug_printf("\nGC BEGINS\n"));

	IF_GC_TRACE(debug_printf("arg1: %s\n",_show_obj_bytes(arg1, tBuf)));
	mark (arg1);
	IF_GC_TRACE(debug_printf("arg2: %s\n",_show_obj_bytes(arg2, tBuf)));
	mark (arg2);
	IF_GC_TRACE(debug_printf("arg3: %s\n",_show_obj_bytes(arg3, tBuf)));
	mark (arg3);
	IF_GC_TRACE(debug_printf("arg4: %s\n",_show_obj_bytes(arg4, tBuf)));
	mark (arg4);
	IF_GC_TRACE(debug_printf("symTable: %s\n",_show_obj_bytes(symTable, tBuf)));
	mark (symTable);
	IF_GC_TRACE(debug_printf("cont: %s\n",_show_obj_bytes(cont, tBuf)));
	mark (cont);
	IF_GC_TRACE(debug_printf("env: %s\n",_show_obj_bytes(env, tBuf)));
	mark (env);

#ifdef CONFIG_BIGNUM_LONG
	bignum_gc_mark();
#endif

	IF_GC_TRACE(debug_printf("globals\n"));

	for (i=0; i<glovars; i++) {
		mark (get_global (i));
	}

	sweep ();
}

obj alloc_ram_cell ()
{
	obj o;

#ifdef CONFIG_GC_AGGRESSIVE
	gc ();
#endif

	if (free_list == 0) {
#ifndef CONFIG_GC_DEBUG
		gc ();

		if (free_list == 0)
#endif
			ERROR("alloc_ram_cell", "memory is full");
	}

	o = free_list;

	free_list = ram_get_car (o);

	return o;
}

obj alloc_ram_cell_init (uint8 f0, uint8 f1, uint8 f2, uint8 f3)
{
	obj o = alloc_ram_cell ();

	ram_set_field0 (o, f0);
	ram_set_field1 (o, f1);
	ram_set_field2 (o, f2);
	ram_set_field3 (o, f3);

	return o;
}

/*
  Vector space layout.

  Vector space is divided into blocks of 4-byte words.
  A block can be free or used, in which case it holds useful data.

  All blocks start with a 4-byte header.
  The GC tag 0 is 0 for free blocks, 1 for used block.
  The car of this header is the size (in 4-byte words, including header) of
  the block.
  Used blocks have a pointer to their header in the object heap in the cdr.

  free_vec_pointer points (using RAM addressing) to the first free word.
  Allocation involves bumping that pointer and setting the header of the new
  used block.
  Freeing is done as part of the object heap GC. Any time a vector header in
  the object heap is freed, the vector space block corresponding to its
  contents is marked as free.
  When the vector space is full (free pointer would go out of bounds after
  an allocation), object heap GC is triggered and the vector space is
  compacted.
 */

void compact ()
{
	/*
	  Move all used blocks to the left.
	  This is done by scanning the heap, and moving any taken block to the
	  left if there's free space before it.
	*/

	obj cur = _SYS_VEC_TO_RAM_OBJ(MIN_VEC_ENCODING);
	obj prev = 0;

	uint16 cur_size;

	while (cur < free_vec_pointer) {
		cur_size  = ram_get_car(cur);
		IF_GC_TRACE(debug_printf("compact: header block %hu (len %hu) has mark %d\n", cur, cur_size, ram_get_gc_tag0(cur)));
#ifndef DEBUG
		if (cur_size == 0) {
			ERROR("gc.compact", "invalid header!");
		}
#endif

		if (prev && !ram_get_gc_tag0 (prev)) { // previous block is free
			if (!ram_get_gc_tag0 (cur)) { // current is free too, merge free spaces
				// advance cur, but prev stays in place
				cur += cur_size;
			} else { // prev is free, but not cur, move cur to start at prev
				// fix header in the object heap to point to the data's new
				// location
				ram_set_cdr(ram_get_cdr(cur), _SYS_RAM_TO_VEC_OBJ(prev+1));
				
				// copy cur's data, which includes header
				// can't use postfix decrement for this because integer
				// promotion rules make it UB
				while ((int16_t)(cur_size = (uint16)(cur_size - 1)) >= 0) {
					ram_set_field0(prev, ram_get_field0(cur));
					ram_set_field1(prev, ram_get_field1(cur));
					ram_set_field2(prev, ram_get_field2(cur));
					ram_set_field3(prev, ram_get_field3(cur));
					cur++;
					prev++;
				}

				// set up a free block where the end of cur's data was
				// (prev is already there from the iteration above)
				ram_set_gc_tag0(prev, 0);
				// at this point, cur is after the new free space, where the
				// next block is
			}
		} else {
			// Go to the next block, which is <size> away from cur.
			prev = cur;
			cur += cur_size;
		}
	}

	if (prev) {
		if(ram_get_gc_tag0(prev)) {
			prev = cur;
		}
		IF_GC_TRACE(debug_printf("free_vec_pointer compacted from %hu to %hu\n", free_vec_pointer, prev));
		// free space is now all at the end
		free_vec_pointer = prev;
	} else {
		IF_GC_TRACE(debug_printf("no vectors allocated, free_vec_pointer remains %hu\n", free_vec_pointer));
	}
	
	
}

// orphan vec cells are never GCed but GC can run
// whenever we allocate so it's important we allocate
// this first to avoid leaving the ram space pointer
// in an invalid state.
obj alloc_vec_cell (uint16 n)
{
	if (n > 0x1fff) {
		TYPE_ERROR("alloc_vec_cell","13-bit fixnum");
	}
	uint8 gc_done = 0;

#ifdef CONFIG_GC_DEBUG
	gc ();
	compact();
	gc_done = 1;
#endif

	// get minimum number of 4-byte blocks (round to nearest 4)
	// this includes a 4-byte vector space header
	n = ((n+3) >> 2) + 1;

	while ((_SYS_VEC_TO_RAM_OBJ(MAX_VEC_ENCODING) - free_vec_pointer) < n) {
		// free space too small, trigger gc
		if (gc_done) { // we gc'd, but no space is big enough for the vector
			ERROR("alloc_vec_cell", "no room for vector");
		}

#ifndef CONFIG_GC_DEBUG
		gc ();
		compact();
		gc_done = 1;
#endif
	}

	obj o = free_vec_pointer;


	IF_GC_TRACE(debug_printf("Bumping free_vec_pointer from %hu to %hu\n", free_vec_pointer, free_vec_pointer + n));
	// advance the free pointer
	free_vec_pointer += n;

	// store block size
	ram_set_car (o, n);
	
	ram_set_gc_tag0 (o, GC_TAG_0_LEFT); // mark block as used

	// n.b. pointer to object-space header is handle after the fact
	// return pointer to header, follow-up code will need to advance this
	return o;
}

#ifdef CONFIG_GC_STATISTICS_PRIMITIVE
PRIMITIVE(#%gc-max-live, gc_max_live, 0)
{
	arg1 = encode_int (max_live);
}
#endif
