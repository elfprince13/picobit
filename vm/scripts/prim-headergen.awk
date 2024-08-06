END {
	print "#include <picobit.h>"
	print "#include <primitives.h>"
	print ""
	print "#ifdef __cplusplus"
	print "extern \"C\" {"
	print "#endif"
	print "#ifdef CONFIG_DEBUG_STRINGS"
	print "const char* const primitive_names[] = {"

	for(i=0; i<prc; i++) {
		print "\t\"" pr[i, "scheme_name"] "\","
	}

	print "};"
	print "#ifdef __cplusplus"
	print "}"
	print "#endif"
	print "#endif /* CONFIG_DEBUG_STRINGS */"
}
