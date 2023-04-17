#include <iostream>
#include "memory.h"
#include "sysbus.h"
#include "arm.h"
#include "clock.h"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    std::cout << "Usage: usim <bin>";
    return 0;
  }
  SystemBus bus;
  Clock clock;

  /* Initialize CPU */
  CortexM0 cpu(clock, &bus);

  /* Initialize Flash */
  Memory flash(1024);
  printf("flash: %s", argv[1]);
  flash.load_bin(argv[1], 0);
  bus.register_device(0x00000000, &flash);

  /* Initialize SRAM */
  Memory sram(1024);
  bus.register_device(0x20000000, &sram);

  clock.init();

  while (true) {
    clock.tick();
  }
}
