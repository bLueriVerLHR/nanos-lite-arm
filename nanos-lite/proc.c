#include "common.h"

void naive_uload(const char *filename);

void init_proc() {
  // load program here
  naive_uload("/bin/bird");
}