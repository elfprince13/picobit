#ifndef PICOBIT_ARCH_EZ80_MEMORY_H
#define PICOBIT_ARCH_EZ80_MEMORY_H
#ifdef __cplusplus
extern "C" {
#endif
#define CODE_START 0x0000

extern uint8 ram_mem[];
#define ram_get(a) ram_mem[a]
#define ram_set(a,x) ram_mem[a] = (x)

#define ROM_BYTES 8192
extern uint8* rom_mem;
#define rom_get(a) (rom_mem[(a)-CODE_START])
#ifdef __cplusplus
}
#endif
#endif