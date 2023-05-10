#include "def.h"

void _start(void) {
  volatile int result = main();
  while (1)
    ;
}