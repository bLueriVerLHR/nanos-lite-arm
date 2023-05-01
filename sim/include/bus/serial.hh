#pragma once

#include "device.hh"

class Serial : public Device {
public:
  Serial(size_t siz);

  void write(char *buf, size_t addr, size_t len);
  void read(char *buf, size_t addr, size_t len);

  void write64(uint64_t &dword, size_t addr);
  void read64(uint64_t &dword, size_t addr);

  void write32(uint32_t &word, size_t addr);
  void read32(uint32_t &word, size_t addr);

  void write16(uint16_t &hword, size_t addr);
  void read16(uint16_t &hword, size_t addr);

  void write8(uint8_t &byte, size_t addr);
  void read8(uint8_t &byte, size_t addr);
};