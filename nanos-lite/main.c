#include "common.h"

void init_device(void);
void init_ramdisk(void);
void init_irq(void);
void init_fs(void);
void init_proc(void);

int main() {
  init_device();

  init_ramdisk();

  init_fs();

  init_proc();

  return 0;
}
