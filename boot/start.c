#include "def.h"

void start(void) {
  volatile int result = main();
  while (1)
    ;
}