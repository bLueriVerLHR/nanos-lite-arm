#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef DEF_H__
#define DEF_H__

// arm asm for Cortex M0
static inline void wfe() { asm volatile("wfe"); }
static inline void wfi() { asm volatile("wfi"); }
static inline void cpsid() { asm volatile("cpsid i"); }
static inline void cpsie() { asm volatile("cpsie i"); }
static inline void sev() { asm volatile("sev"); }
static inline void nop() { asm volatile("nop"); }
static inline void dmb() { asm volatile("dmb"); }
static inline void dsb() { asm volatile("dsb"); }
static inline void isb() { asm volatile("isb"); }

#define svc(imm)                                                               \
  do {                                                                         \
    asm volatile("svc"                                                         \
                 " #" #imm);                                                   \
  } while (0)

// main.c
void start(void);
int main(void);

//
// misc
//

void do_syscall(uintptr_t a[4]);

#endif
