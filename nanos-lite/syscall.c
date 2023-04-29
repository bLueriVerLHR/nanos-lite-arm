#include <sys/time.h>
#include <time.h>

#include "common.h"
#include "fs.h"
#include "../syscall/syscall.h"
#include "../boot/def.h"

uintptr_t do_syscall(uintptr_t r0, uintptr_t r1, uintptr_t r2, uintptr_t r3) {
  switch (r0) {
    case SYS_yield: yield();                                break;
    case SYS_exit:  nop();                                break;
    case SYS_open:  return fs_open ((char *)r1, r2, r3);  break;
    case SYS_write: return fs_write(r1, (void *)r2, r3);  break;
    case SYS_brk:                                         break;
    case SYS_read:  return fs_read (r1, (void *)r2, r3);  break;
    case SYS_close: return fs_close(r1);                  break;
    case SYS_lseek: return fs_lseek(r1, r2, r3);          break;
    case SYS_gettimeofday: {
      if (r1 == 0) break;
      struct timeval *tv = (struct timeval *)r1;
      uint64_t us = io_read(AM_TIMER_UPTIME).us;
      tv->tv_usec = us % 1000000;
      tv->tv_sec  = us / 1000000;
    } break;
    default: nop();
  }
  return 0;
}
