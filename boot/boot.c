#include "def.h"

#define STACK_BASE 0x20020000
#define STACK_SIZE 0x8000
#define HEAP_BASE 0x20020000
#define HEAP_SIZE 0x10000 - STACK_SIZE

enum { MSP = 0, RST = 1, NMI = 2, HARDWARE_ERROR = 3, SVC = 11 };

volatile uintptr_t idt[] __attribute__((aligned(4), section("Exceptions"))) = {
  [MSP] = (uintptr_t)STACK_BASE,
  [RST] = (uintptr_t)start,
  [SVC] = (uintptr_t)do_syscall,
};

volatile char endofbin[0] __attribute__((aligned(4), section("EoB")));

volatile char *_end = endofbin;