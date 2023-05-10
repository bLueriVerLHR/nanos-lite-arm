#pragma once

#include <cstdint>
#include <cstdlib>

class Device {
protected:
  size_t devsiz;

public:
  Device(size_t siz);

  const size_t &size();

  virtual void write(char *buf, size_t addr, size_t len) = 0;
  virtual void read(char *buf, size_t addr, size_t len) = 0;

  virtual void write64(uint64_t &dword, size_t addr) = 0;
  virtual void read64(uint64_t &dword, size_t addr) = 0;

  virtual void write32(uint32_t &word, size_t addr) = 0;
  virtual void read32(uint32_t &word, size_t addr) = 0;

  virtual void write16(uint16_t &hword, size_t addr) = 0;
  virtual void read16(uint16_t &hword, size_t addr) = 0;

  virtual void write8(uint8_t &byte, size_t addr) = 0;
  virtual void read8(uint8_t &byte, size_t addr) = 0;

  virtual ~Device() = default;
};