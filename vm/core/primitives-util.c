#include <picobit.h>
#include <primitives.h>
#include <gc.h>

PRIMITIVE(eq?, eq_p, 2)
{
	arg1 = encode_bool (arg1 == arg2);
	arg2 = OBJ_FALSE;
}

PRIMITIVE(not, not, 1)
{
	arg1 = encode_bool (arg1 == OBJ_FALSE);
}

PRIMITIVE(symbol?, symbol_p, 1)
{
	if (IN_RAM(arg1)) {
		arg1 = encode_bool (RAM_SYMBOL_P(arg1));
	} else if (IN_ROM(arg1)) {
		arg1 = encode_bool (ROM_SYMBOL_P(arg1));
	} else {
		arg1 = OBJ_FALSE;
	}
}

PRIMITIVE(boolean?, boolean_p, 1)
{
	arg1 = encode_bool (arg1 < 2);
}
