ifdef_any_of = $(filter-out undefined,$(foreach v,$(1),$(origin $(v))))

ifneq ($(call ifdef_any_of,CONFIG_VM_DEBUG CONFIG_GC_DEBUG),)
CFLAGS += -g3 -Og -fsanitize=undefined,bounds,address,nullability,pointer-overflow,integer
endif


arch-y   += entry.c primitives.c
stdlib-y += stdlib.scm

load-address := \#x0000

ifdef CONFIG_LITTLE_ENDIAN
conf-suffix := -le
else
conf-suffix := -be
endif

arch-all: picobit-vm$(conf-suffix)

picobit-vm$(conf-suffix): $(obj-y) .config
	clang $(CFLAGS) -g -o $@ $(obj-y)
