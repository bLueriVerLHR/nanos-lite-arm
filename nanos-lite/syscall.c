#include <sys/time.h>
#include <time.h>

#include "common.h"
#include "fs.h"
#include "../syscall/syscall.h"
#include "../def.h"

void do_syscall(uintptr_t a[4]) {
  switch (a[0]) {
    case SYS_yield: nop();                                      break;
    case SYS_exit:  nop();                                      break;
    case SYS_open:  a[0] = fs_open ((char *)a[1], a[2], a[3]);  break;
    case SYS_write: a[0] = fs_write(a[1], (void *)a[2], a[3]);  break;
    case SYS_brk:   a[0] = 0;                                   break;
    case SYS_read:  a[0] = fs_read (a[1], (void *)a[2], a[3]);  break;
    case SYS_close: a[0] = fs_close(a[1]);                      break;
    case SYS_lseek: a[0] = fs_lseek(a[1], a[2], a[3]);          break;
    case SYS_gettimeofday:
      if (a[1]) {
        struct timeval *tv = (struct timeval *)a[1];
        uint64_t us = io_read(AM_TIMER_UPTIME).us;
        tv->tv_usec = us % 1000000;
        tv->tv_sec  = us / 1000000;
      }
      a[0] = 0;
      break;
    default: nop();
  }
}
