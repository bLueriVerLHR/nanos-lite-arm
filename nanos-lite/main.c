#include "common.h"
#include "../boot/def.h"
#include "../am/devaddr.h"

void init_device(void);
void init_ramdisk(void);
void init_irq(void);
void init_fs(void);
void init_proc(void);

void yield();

int main() {
  puts("Hello world!\n");
  fflush(stdout);
  
  yield();
  
  init_device();

  init_ramdisk();

  init_fs();

  // init_proc();

  return 0;
}
