#include <cstdint>
#include <string>

#include "device.h"

#ifndef SERIAL_H
#define SERIAL_H

class Serial : public DeviceIf {
public:
  uint32_t read32(uint32_t addr);
  void write32(uint32_t addr, uint32_t data);
  uint32_t get_size();
};

#endif