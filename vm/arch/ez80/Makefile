ifdef_any_of = $(filter-out undefined,$(foreach v,$(1),$(origin $(v))))

ifneq ($(call ifdef_any_of,CONFIG_VM_DEBUG CONFIG_GC_DEBUG),)
DEBUGMODE = DEBUG
# there appears to be an issue where app_tools isn't setting up debug_frame properly?
CCDEBUG = -g0 #-gdwarf-5 -g3
LDDEBUG = 1
else
DEBUGMODE = NDEBUG
CCDEBUG = -g0
LDDEBUG = 0
endif

ON_CALC_NAME := "Rustle"
ON_CALC_DESC := "A small Scheme dialect"

CEDEV := $(shell cedev-config --prefix)

CFLAGS += -MD -c -emit-llvm -nostdinc -isystem $(CEDEV)/include -fno-threadsafe-statics -Xclang -fforce-mangle-main-argc-argv -mllvm -profile-guided-section-prefix=false -D$(DEBUGMODE) $(CCDEBUG) -Wall -Wextra -Oz -flto

ifdef CONFIG_CUSTOM_DEBUG_PRINTF
CFLAGS += -D"debug_printf(...)=sprintf(((char*)0xFB0000),\#\#__VA_ARGS__)"
endif

CXXFLAGS = $(subst -nostdinc, -nostdinc++, $(subst -std=gnu11, -std=gnu++17, $(CFLAGS))) -isystem $(CEDEV)/include/c++ -D_cplusplus -fno-exceptions -fno-rtti -fno-use-cxa-atexit

# only .c extensions will be preprocessed into .primitives.p
arch-y   += entry.cpp memory.cpp menu.cpp primitives.c
stdlib-y += stdlib.scm

load-address := \#x0000

arch-all: rustle.8ek

llvm-c-y = $(addsuffix .bc,$(filter %.c,$(obj-y)))
llvm-cpp-y = $(addsuffix .bc,$(filter %.cpp,$(obj-y)))

$(llvm-c-y): %.c.bc: %.c .config arch/ez80/include/memory.h arch/ez80/include/exit-codes.h arch/ez80/include/types.h arch/ez80/include/cemu-debug.h $(core-h)
	$(CROSS)clang $(CFLAGS) $< -o $@

$(llvm-cpp-y): %.cpp.bc: %.cpp .config arch/ez80/include/menu.h arch/ez80/include/exit-codes.h arch/ez80/include/memory.h arch/ez80/include/cemu-debug.h $(core-h)
	$(CROSS)clang $(CXXFLAGS) $< -o $@

picobit-vm.bc: $(llvm-c-y) $(llvm-cpp-y) .config
	$(CROSS)link $(llvm-c-y) $(llvm-cpp-y) -o $@

picobit-vm.ll: picobit-vm.bc
	$(CROSS)clang -emit-llvm -c -S -mllvm -profile-guided-section-prefix=false -Wall -Wextra -Oz picobit-vm.bc -o $@

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
		-i 'DEBUG := $(LDDEBUG)' -i 'HAS_PRINTF := 1' -i 'HAS_LIBC := 1' -i 'HAS_LIBCXX := 1' -i 'PREFER_OS_CRT := 0' -i 'PREFER_OS_LIBC := 1' -i 'ALLOCATOR_STANDARD := 1'\
		-i 'include "linker_script"'\
		-i 'range .bss $$D052C6 : $$D13FD8' -i 'provide __stack = $$D1A87E' -i 'locate .header at $$0'\
		-i 'provide __app_name = $(ON_CALC_NAME)'  -i 'provide __app_version = "0"' -i 'provide __app_desc = $(ON_CALC_DESC)'\
		-i 'provide _rom_mem = $$$(CONFIG_ROM_MEM_PTR)'\
		-i 'source "icon.src", "$(CEDEV)/lib/crt/crt0.src", "picobit-vm.src"'\
		-i 'library "$(CEDEV)/lib/libload/fatdrvce.lib", "$(CEDEV)/lib/libload/fileioc.lib", "$(CEDEV)/lib/libload/fontlibc.lib", "$(CEDEV)/lib/libload/graphx.lib", "$(CEDEV)/lib/libload/keypadc.lib", "$(CEDEV)/lib/libload/msddrvce.lib", "$(CEDEV)/lib/libload/srldrvce.lib", "$(CEDEV)/lib/libload/usbdrvce.lib"'\
		'$@'

