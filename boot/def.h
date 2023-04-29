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
static inline void yield() { asm volatile("yield"); }

#define svc(code) asm volatile ("svc %[immediate]"::[immediate] "I" (code))

#define bkpt(code) asm volatile ("bkpt %[immediate]"::[immediate] "I" (code))


// main.c
void start(void);
int main(void);

//
// misc
//

uintptr_t do_syscall(uintptr_t r0, uintptr_t r1, uintptr_t r2, uintptr_t r3);

#endif
