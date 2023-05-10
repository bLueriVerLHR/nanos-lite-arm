#include <stdio.h>

#include "xdef.hh"
#include "common.hh"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: sim <bin>";
    return 0;
  }

  SystemBus bus;
  Memory ram(2 * 1024 * 1024);
  Memory stk(256 * 1024);

  Memory flash(1024 * 1024);
  flash.load(argv[1]);
  ram.load(&flash, IMG_ADDR, flash.size());

  bus.regdev(&ram,  RAM_ADDR);
  bus.regdev(&stk,  STK_ADDR);

  Serial bios(1);
  bus.regdev(&bios, SERIAL_PORT);

  Gcpu *cpu = new Cortex_M0(&bus);

  while (true) {
    cpu->Step(1);
  }

  delete cpu;
  return 0;
}