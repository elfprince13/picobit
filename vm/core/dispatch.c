#include <picobit.h>
#include <dispatch.h>
#include <debug.h>
#include <gc.h>
#include <primitives.h>

#ifndef NO_PRIMITIVE_EXPAND
#include <gen.primitives.h>
#endif /* NO_PRIMITIVE_EXPAND */

#ifdef CONFIG_LITTLE_ENDIAN
#define OP_ARGS_TO_ENTRY(LO8, HI8) (((uint16)(HI8) << 8) | (LO8))
#define OP_ARGS_TO_ENTRY12(LO4, HI8) (((uint16)(HI8) << 4) | (LO4))
#else
#define OP_ARGS_TO_ENTRY(HI8, LO8) (((uint16)(HI8) << 8) | (LO8))
#define OP_ARGS_TO_ENTRY12(HI4, LO8) (((uint16)(HI4) << 8) | (LO8))
#endif

/*
 * This pragma turns off GCC warning/error about implicit declaration
 * of primitives.
 */
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

/*
 * To avoid creating common symbols and linkage weirdness, interpreter
 * variables are defined here and declared as extern in the header.
 */

obj cont, env;
obj arg1, arg2, arg3, arg4;
obj symTable;
obj a1, a2, a3;

rom_addr pc, entry;
uint8 glovars;

void push_arg1 ()
{
	IF_TRACE((debug_printf("push_arg1 [before]: "), show_obj_bytes(env), debug_printf("\n")));
	env = cons (arg1, env);
	IF_TRACE((debug_printf("push_arg1 [ after]: "), show_obj_bytes(env), debug_printf("\n")));
	arg1 = OBJ_FALSE;
}

obj pop ()
{
	obj o = ram_get_car (env);
	env = ram_get_cdr (env);
	return o;
}

void pop_procedure ()
{
	arg1 = pop();

	if (IN_RAM(arg1)) {
		if (!RAM_CLOSURE_P(arg1)) {
			TYPE_ERROR("pop_procedure.0", "procedure");
		}

		entry = ram_get_entry (arg1) + CODE_START;
	} else {
		TYPE_ERROR("pop_procedure.1", "procedure");
	}
}

uint8 handle_arity_and_rest_param (uint8 na)
{
	uint8 np;

	np = rom_get (entry++);

	if (arg1 != OBJ_NULL) {
		arg1 = ram_get_car(arg1);        // closed environment
	}

	if ((np & 0x80) == 0) {
		if (na != np) {
			ERROR("handle_arity_and_rest_param.0", "wrong number of arguments");
		}
	} else {
		np = (uint8)(~np);

		if (na < np) {
			ERROR("handle_arity_and_rest_param.1", "wrong number of arguments");
		}

		arg3 = OBJ_NULL;

		while (na > np) {
			arg4 = pop();

			arg3 = cons (arg4, arg3);
			arg4 = OBJ_FALSE;

			na--;
		}

		arg1 = cons (arg3, arg1);
		arg3 = OBJ_FALSE;
	}

	return na;
}

void build_env (uint8 na)
{
	while (na != 0) {
		arg3 = pop();

		arg1 = cons (arg3, arg1);

		na--;
	}

	arg3 = OBJ_FALSE;
}

void save_cont ()
{
	// the second half is a closure
	arg3 = alloc_ram_cell_init (CLOSURE_FIELD0 | (env >> 8),
	                            env & 0xff,
	                            (pc >> 8),
	                            (pc & 0xff));
	cont = alloc_ram_cell_init (COMPOSITE_FIELD0 | (cont >> 8),
	                            cont & 0xff,
	                            CONTINUATION_FIELD2 | (arg3 >> 8),
	                            arg3 & 0xff);
	arg3 = OBJ_FALSE;
}

static uint8 bytecode, bytecode_hi4, bytecode_lo4;

void interpreter ()
{
	{
		uint8 numConstants = (256 *
#ifdef CONFIG_LITTLE_ENDIAN
			(rom_get (CODE_START+0) & 0x07)
#else
		    (rom_get (CODE_START+1) & 0x07)
#endif
			) + rom_get(CODE_START + 2);
		if ((numConstants + MIN_ROM_ENCODING - 1) >= MIN_RAM_ENCODING) {
			ERROR("interpreter", "too many constants");
		}
		glovars = rom_get (CODE_START+3); // number of global variables
		// does not depend on pc
		init_ram_heap ();
		init_sym_table(numConstants);

		pc = (CODE_START + 4) + (((uint16)numConstants) << 2);
	}
	


dispatch:
	IF_TRACE(show_state (pc));
	FETCH_NEXT_BYTECODE();
	bytecode_hi4 = bytecode & 0xf0;
	bytecode_lo4 = bytecode & 0x0f;

	switch (bytecode_hi4 >> 4) {

		/*************************************************************************/
	case PUSH_CONSTANT1 :

		IF_TRACE(debug_printf("  (push-constant "); show_obj (bytecode_lo4); debug_printf("="); show_obj_bytes(bytecode_lo4); debug_printf (")\n"));

		arg1 = bytecode_lo4;

		push_arg1();

		goto dispatch;

		/*************************************************************************/
	case PUSH_CONSTANT2 :

		IF_TRACE(debug_printf("  (push-constant "); show_obj (bytecode_lo4+16); debug_printf("="); show_obj_bytes(bytecode_lo4+16); debug_printf (")\n"));
		arg1 = bytecode_lo4+16;

		push_arg1();

		goto dispatch;

		/*************************************************************************/
	case PUSH_STACK1 :
		// TODO this is painfully slow :sob:
		// switch to a vector-based stack that grows from the end of vector space
		IF_TRACE(debug_printf("  (push-stack %d)\n", bytecode_lo4));

		arg1 = env;

		while (bytecode_lo4 != 0) {
			arg1 = ram_get_cdr (arg1);
			bytecode_lo4--;
		}

		arg1 = ram_get_car (arg1);

		push_arg1();

		goto dispatch;

		/*************************************************************************/
	case PUSH_STACK2 :
		// TODO this is painfully slow :sob:
		// switch to a vector-based stack that grows from the end of vector space

		IF_TRACE(debug_printf("  (push-stack %d)\n", bytecode_lo4+16));

		bytecode_lo4 += 16;

		arg1 = env;

		while (bytecode_lo4 != 0) {
			arg1 = ram_get_cdr (arg1);
			bytecode_lo4--;
		}

		arg1 = ram_get_car (arg1);

		push_arg1();

		goto dispatch;

		/*************************************************************************/
	case PUSH_GLOBAL :

		IF_TRACE(debug_printf("  (push-global %d)\n", bytecode_lo4));

		arg1 = get_global (bytecode_lo4);

		push_arg1();

		goto dispatch;

		/*************************************************************************/
	case SET_GLOBAL :

		IF_TRACE(debug_printf("  (set-global %d)\n", bytecode_lo4));

		set_global (bytecode_lo4, pop());

		goto dispatch;

		/*************************************************************************/
	case CALL :

		IF_TRACE(debug_printf("  (call %d)\n", bytecode_lo4));

		pop_procedure ();
		build_env (handle_arity_and_rest_param (bytecode_lo4));
		save_cont ();

		env = arg1;
		pc = entry;

		arg1 = OBJ_FALSE;

		goto dispatch;

		/*************************************************************************/
	case JUMP :

		IF_TRACE(debug_printf("  (jump %d)\n", bytecode_lo4));

		pop_procedure ();
		build_env (handle_arity_and_rest_param (bytecode_lo4));

		env = arg1;
		pc = entry;

		arg1 = OBJ_FALSE;

		goto dispatch;

		/*************************************************************************/
	case LABEL_INSTR :

		switch (bytecode_lo4) {
		case 0: // call-toplevel
			FETCH_NEXT_BYTECODE();
			arg2 = bytecode;

			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (call-toplevel 0x%04x)\n",
			                OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START));

			entry = OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START;
			arg1 = OBJ_NULL;
			arg2 = OBJ_FALSE;

			build_env (rom_get (entry++));
			save_cont ();

			env = arg1;
			pc = entry;

			arg1 = OBJ_FALSE;
			
			break;

		case 1: // jump-toplevel
			FETCH_NEXT_BYTECODE();
			arg2 = bytecode;

			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (jump-toplevel 0x%04x)\n",
			                OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START));

			entry = OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START;
			arg1 = OBJ_NULL;
			arg2 = OBJ_FALSE;

			build_env (rom_get (entry++));

			env = arg1;
			pc = entry;

			arg1 = OBJ_FALSE;

			break;

		case 2: // goto
			FETCH_NEXT_BYTECODE();
			arg2 = bytecode;

			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (goto 0x%04x)\n",
			                OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START));

			pc = OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START;

			break;

		case 3: // goto-if-false
			FETCH_NEXT_BYTECODE();
			arg2 = bytecode;

			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (goto-if-false 0x%04x)\n",
			                OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START));

			if (pop() == OBJ_FALSE) {
				pc = OP_ARGS_TO_ENTRY(arg2, bytecode) + CODE_START;
			}

			break;

		case 4: // closure
			FETCH_NEXT_BYTECODE();
			arg2 = bytecode;

			FETCH_NEXT_BYTECODE();
			entry = OP_ARGS_TO_ENTRY(arg2, bytecode);
			arg2 = OBJ_FALSE;

			IF_TRACE(debug_printf("  (closure 0x%04x)\n", entry));

			arg3 = pop(); // env

			arg1 = alloc_ram_cell_init (CLOSURE_FIELD0 | (arg3 >> 8),
			                            arg3 & 0xff,
			                            entry >> 8,
			                            (entry & 0xff));

			arg3 = OBJ_FALSE;
			push_arg1();


			break;

#if 1
		case 5: // call-toplevel-rel8
			FETCH_NEXT_BYTECODE(); // TODO the short version have a lot in common with the long ones, abstract ?

			IF_TRACE(debug_printf("  (call-toplevel-rel8 0x%04x)\n", pc + bytecode - 128));

			entry = pc + bytecode - 128;
			arg1 = OBJ_NULL;

			build_env (rom_get (entry++));
			save_cont ();

			env = arg1;
			pc = entry;

			arg1 = OBJ_FALSE;

			break;

		case 6: // jump-toplevel-rel8
			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (jump-toplevel-rel8 0x%04x)\n", pc + bytecode - 128));

			entry = pc + bytecode - 128;
			arg1 = OBJ_NULL;

			build_env (rom_get (entry++));

			env = arg1;
			pc = entry;

			arg1 = OBJ_FALSE;

			break;

		case 7: // goto-rel8
			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (goto-rel8 0x%04x)\n", pc + bytecode - 128));

			pc = pc + bytecode - 128;

			break;

		case 8: // goto-if-false-rel8
			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (goto-if-false-rel8 0x%04x)\n", pc + bytecode - 128));

			if (pop() == OBJ_FALSE) {
				pc = pc + bytecode - 128;
			}

			break;

		case 9: // closure-rel8
			FETCH_NEXT_BYTECODE();

			entry = pc - CODE_START + bytecode - 128;

			IF_TRACE(debug_printf("  (closure-rel8 0x%04x)\n", entry));

			arg3 = pop(); // env

			arg1 = alloc_ram_cell_init (CLOSURE_FIELD0 | (arg3 >> 8),
			                            arg3 & 0xff,
			                            entry >> 8,
			                            (entry & 0xff));

			push_arg1();

			arg3 = OBJ_FALSE;

			break;
#endif

#if 0
		case 10: // FREE
			break;
		case 11:
			break;
		case 12:
			break;
#endif
		case 13: // push_stack [long]
			// TODO this is painfully slow :sob:
			// switch to a vector-based stack that grows from the end of vector space
			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (push-stack [long] %d)\n", bytecode));

			arg1 = env;

			while (bytecode != 0) {
				arg1 = ram_get_cdr (arg1);
				bytecode--;
			}

			arg1 = ram_get_car (arg1);

			push_arg1();


			break;
		case 14: // push_global [long]
			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (push-global [long] %d)\n", bytecode));

			arg1 = get_global (bytecode);

			push_arg1();

			break;

		case 15: // set_global [long]
			FETCH_NEXT_BYTECODE();

			IF_TRACE(debug_printf("  (set-global [long] %d)\n", bytecode));

			set_global (bytecode, pop());

			break;
		}

		goto dispatch;

		/*************************************************************************/
	case PUSH_CONSTANT_LONG :

		/* push-constant [long] */

		FETCH_NEXT_BYTECODE();

		IF_TRACE((debug_printf("  (push [long] "), show_obj_bytes(OP_ARGS_TO_ENTRY12(bytecode_lo4, bytecode)), debug_printf(")\n")));

		// necessary since SIXPIC would have kept the result of the shift at 8 bits
		arg1 = bytecode_lo4;
		arg1 = OP_ARGS_TO_ENTRY12(bytecode_lo4, bytecode);
		push_arg1();

		goto dispatch;

		/*************************************************************************/

	case JUMP_TOPLEVEL_REL4 :

		IF_TRACE(debug_printf("  (jump-toplevel-rel4 0x%04x)\n", pc + (bytecode & 0x0f)));

		entry = pc + (bytecode & 0x0f);
		arg1 = OBJ_NULL;

		build_env (rom_get (entry++));

		env = arg1;
		pc = entry;

		arg1 = OBJ_FALSE;

		goto dispatch;

		/*************************************************************************/

	case GOTO_IF_FALSE_REL4 :

		IF_TRACE(debug_printf("  (goto-if-false-rel4 0x%04x)\n", pc + (bytecode & 0x0f)));

		if (pop() == OBJ_FALSE) {
			pc = pc + (bytecode & 0x0f);
		}

		goto dispatch;

	#ifndef NO_PRIMITIVE_EXPAND
	#include "gen.dispatch.c"
	#endif /* NO_PRIMITIVE_EXPAND */
	}
}
