#include <cassert>

#include "serial.h"

void Serial::write32(uint32_t addr, uint32_t data) {
  assert(addr == SERIAL_PORT);
  putchar(data);
}

uint32_t Serial::read32(uint32_t addr) {
  assert(0 && "unreadable");
  return 0;
}

uint32_t Serial::get_size() {
  return sizeof(char);
}