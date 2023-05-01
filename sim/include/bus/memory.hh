#pragma once

#include "device.hh"

class Memory : public Device {
  char *data;
  bool wen;
  bool ren;
  bool xen;
public:
  Memory(size_t siz, bool w = true, bool r = true, bool x = true);
  ~Memory();

  void load(const char *path);
  void load(Memory *mem, size_t addr, size_t len);

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