#include "bus/memory.hh"
#include "common.hh"

Memory::Memory(size_t siz, bool w, bool r, bool x)
    : Device(siz), wen(w), ren(r), xen(x) {
  data = new char[siz];
  memset((void *)data, 0, siz);
}

void Memory::load(const char *path) {
  std::fstream ifs(path, std::ios::in | std::ios::binary);
  panicifnot(ifs);
  ifs.read(data, devsiz);
  ifs.close();
}

void Memory::load(Memory *mem, size_t addr, size_t len) {
  panicifnot(addr + len < devsiz);
  memcpy(&data[addr], mem->data, len);
}

void Memory::write(char *buf, size_t addr, size_t len) {
  if (!wen)
    panic("permission denied");
  size_t actlen = addr + len >= devsiz ? devsiz - addr : len;
  memcpy(&data[addr], buf, actlen);
}

void Memory::read(char *buf, size_t addr, size_t len) {
  if (!ren)
    panic("permission denied");
  size_t actlen = addr + len >= devsiz ? devsiz - addr : len;
  memcpy(buf, &data[addr], actlen);
}

Memory::~Memory() { delete[] data; }

void Memory::write64(uint64_t &dword, size_t addr) {
  uint8_t buf[sizeof(dword)];
  buf[0] = (dword >> 0) & 0xFF;
  buf[1] = (dword >> 8) & 0xFF;
  buf[2] = (dword >> 16) & 0xFF;
  buf[3] = (dword >> 24) & 0xFF;
  buf[4] = (dword >> 32) & 0xFF;
  buf[5] = (dword >> 40) & 0xFF;
  buf[6] = (dword >> 48) & 0xFF;
  buf[7] = (dword >> 56) & 0xFF;
  write((char *)buf, addr, sizeof(dword));
}

void Memory::read64(uint64_t &dword, size_t addr) {
  uint8_t buf[sizeof(dword)];
  read((char *)buf, addr, sizeof(dword));
  dword = (uint64_t)buf[0];
  dword |= (uint64_t)buf[1] << 8;
  dword |= (uint64_t)buf[2] << 16;
  dword |= (uint64_t)buf[3] << 24;
  dword |= (uint64_t)buf[4] << 32;
  dword |= (uint64_t)buf[5] << 40;
  dword |= (uint64_t)buf[6] << 48;
  dword |= (uint64_t)buf[7] << 56;
}

void Memory::write32(uint32_t &word, size_t addr) {
  uint8_t buf[sizeof(word)];
  buf[0] = (word >> 0) & 0xFF;
  buf[1] = (word >> 8) & 0xFF;
  buf[2] = (word >> 16) & 0xFF;
  buf[3] = (word >> 24) & 0xFF;
  write((char *)buf, addr, sizeof(word));
}

void Memory::read32(uint32_t &word, size_t addr) {
  uint8_t buf[sizeof(word)];
  read((char *)buf, addr, sizeof(word));
  word = buf[0];
  word |= buf[1] << 8;
  word |= buf[2] << 16;
  word |= buf[3] << 24;
}

void Memory::write16(uint16_t &hword, size_t addr) {
  uint8_t buf[sizeof(hword)];
  buf[0] = (hword >> 0) & 0xFF;
  buf[1] = (hword >> 8) & 0xFF;
  write((char *)buf, addr, sizeof(hword));
}

void Memory::read16(uint16_t &hword, size_t addr) {
  uint8_t buf[sizeof(hword)];
  read((char *)buf, addr, sizeof(hword));
  hword = buf[0];
  hword |= buf[1] << 8;
}

void Memory::write8(uint8_t &byte, size_t addr) {
  write((char *)&byte, addr, sizeof(byte));
}

void Memory::read8(uint8_t &byte, size_t addr) {
  read((char *)&byte, addr, sizeof(byte));
}