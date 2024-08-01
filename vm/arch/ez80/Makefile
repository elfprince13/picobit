ifdef_any_of = $(filter-out undefined,$(foreach v,$(1),$(origin $(v))))

ifneq ($(call ifdef_any_of,CONFIG_VM_DEBUG CONFIG_GC_DEBUG),)
DEBUGMODE = DEBUG
CCDEBUG = -gdwarf-5 -g3
LDDEBUG = 1
else
DEBUGMODE = NDEBUG
CCDEBUG = -g0
LDDEBUG = 0
endif

ON_CALC_NAME := "Rustle"
ON_CALC_DESC := "A small Scheme dialect"

CEDEV := $(shell cedev-config --prefix)

CFLAGS += -MD -c -emit-llvm -nostdinc -isystem $(CEDEV)/include -fno-threadsafe-statics -Xclang -fforce-mangle-main-argc-argv -mllvm -profile-guided-section-prefix=false -D$(DEBUGMODE) $(CCDEBUG) -Wall -Wextra -Oz
arch-y   += entry.c primitives.c
stdlib-y += stdlib.scm

load-address := \#x8000

arch-all: rustle.8ek

llvm-y = $(addsuffix .bc,$(obj-y))

$(llvm-y): %.c.bc: %.c .config
	$(CROSS)clang $(CFLAGS) $< -o $@

picobit-vm.bc: $(llvm-y) .config
	$(CROSS)link $(llvm-y) -o $@

linker_script: arch/ez80/deps/app_tools/linker_script
	CEDEV=$(CEDEV) APP_TOOLS_INSTALL_DIR=arch/ez80/deps/app_tools envsubst < arch/ez80/deps/app_tools/linker_script > linker_script

picobit-vm.src: picobit-vm.bc
	$(CROSS)clang -S -mllvm -profile-guided-section-prefix=false -Wall -Wextra -Oz picobit-vm.bc -o picobit-vm.src

icon.src:
	$(CEDEV)/bin/convimg --icon-output '$@' --icon-format asm --icon-description $(ON_CALC_DESC)

rustle.8ek: picobit-vm.bin
	arch/ez80/deps/app_tools/make_8ek.py picobit-vm.bin rustle.8ek $(ON_CALC_NAME)

picobit-vm.bin: picobit-vm.src icon.src linker_script
	$(CEDEV)/bin/fasmg -n '$(CEDEV)/meta/ld.alm'\
		-i 'DEBUG := 0' -i 'HAS_PRINTF := 1' -i 'HAS_LIBC := 1' -i 'HAS_LIBCXX := 0' -i 'PREFER_OS_CRT := 0' -i 'PREFER_OS_LIBC := 1' -i 'ALLOCATOR_STANDARD := 1'\
		-i 'include "linker_script"'\
		-i 'range .bss $$D052C6 : $$D13FD8' -i 'provide __stack = $$D1A87E' -i 'locate .header at $$0'\
		-i 'provide __app_name = $(ON_CALC_NAME)'  -i 'provide __app_version = "0"' -i 'provide __app_desc = $(ON_CALC_DESC)'\
		-i 'source "icon.src", "$(CEDEV)/lib/crt/crt0.src", "picobit-vm.src"'\
		-i 'library "$(CEDEV)/lib/libload/fatdrvce.lib", "$(CEDEV)/lib/libload/fileioc.lib", "$(CEDEV)/lib/libload/fontlibc.lib", "$(CEDEV)/lib/libload/graphx.lib", "$(CEDEV)/lib/libload/keypadc.lib", "$(CEDEV)/lib/libload/msddrvce.lib", "$(CEDEV)/lib/libload/srldrvce.lib", "$(CEDEV)/lib/libload/usbdrvce.lib"'\
		'$@'

