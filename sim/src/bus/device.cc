#include "bus/device.hh"
#include "common.hh"

Device::Device(size_t siz) : devsiz(siz) {}

const size_t &Device::size() { return devsiz; }

// void Device::write(char *buf, size_t addr, size_t len) {}

// void Device::read(char *buf, size_t addr, size_t len) {}

// void Device::write64(uint64_t &dword, size_t addr) {
//   uint8_t buf[sizeof(dword)];
//   buf[0] = (dword >>  0) & 0xFF;
//   buf[1] = (dword >>  8) & 0xFF;
//   buf[2] = (dword >> 16) & 0xFF;
//   buf[3] = (dword >> 24) & 0xFF;
//   buf[4] = (dword >> 32) & 0xFF;
//   buf[5] = (dword >> 40) & 0xFF;
//   buf[6] = (dword >> 48) & 0xFF;
//   buf[7] = (dword >> 56) & 0xFF;
//   write((char *)buf, addr, sizeof(dword));
// }

// void Device::read64(uint64_t &dword, size_t addr) {
//   uint8_t buf[sizeof(dword)];
//   read((char *)buf, addr, sizeof(dword));
//   dword = buf[0];
//   dword |= buf[1] << 8;
//   dword |= buf[2] << 16;
//   dword |= buf[3] << 24;
//   dword |= buf[4] << 32;
//   dword |= buf[5] << 40;
//   dword |= buf[6] << 48;
//   dword |= buf[7] << 56;
// }

// void Device::write32(uint32_t &word, size_t addr) {
//   uint8_t buf[sizeof(word)];
//   buf[0] = (word >>  0) & 0xFF;
//   buf[1] = (word >>  8) & 0xFF;
//   buf[2] = (word >> 16) & 0xFF;
//   buf[3] = (word >> 24) & 0xFF;
//   write((char *)buf, addr, sizeof(word));
// }

// void Device::read32(uint32_t &word, size_t addr) {
//   uint8_t buf[sizeof(word)];
//   read((char *)buf, addr, sizeof(word));
//   word = buf[0];
//   word |= buf[1] << 8;
//   word |= buf[2] << 16;
//   word |= buf[3] << 24;
// }

// void Device::write16(uint16_t &hword, size_t addr) {
//   uint8_t buf[sizeof(hword)];
//   buf[0] = (hword >>  0) & 0xFF;
//   buf[1] = (hword >>  8) & 0xFF;
//   write((char *)buf, addr, sizeof(hword));
// }

// void Device::read16(uint16_t &hword, size_t addr) {
//   uint8_t buf[sizeof(hword)];
//   read((char *)buf, addr, sizeof(hword));
//   hword = buf[0];
//   hword |= buf[1] << 8;
// }

// void Device::write8(uint8_t &byte, size_t addr) {
//   write((char *)&byte, addr, sizeof(byte));
// }

// void Device::read8(uint8_t &byte, size_t addr) {
//   read((char *)&byte, addr, sizeof(byte));
// }