#include "common.h"

size_t serial_write(const void *buf, size_t offset, size_t len) {
  volatile char *serial_port = (char *)SERIAL_PORT;
  for (size_t i = 0; i < len; ++i) {
    *serial_port = ((char *)buf)[i];
  }
  return 0;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  int w = io_read(AM_GPU_CONFIG).width;
  int h = io_read(AM_GPU_CONFIG).height;

  int ret = sprintf(buf, "%d,%d", w, h);
  if (ret >= len) {
    assert(0);
  }
  
  return ret + 1;
}

#define MMIO_BASE       0xa0000000
#define FB_ADDR         (MMIO_BASE + 0x1000000)
#define SYNC_ADDR       (VGACTL_ADDR + 4)

size_t fb_write(const void *buf, size_t offset, size_t len) {
  const char *src = (const char *)buf;
  char *fb = (char *)(uintptr_t)(FB_ADDR + offset);
  for (int i = 0; i < len; ++i){
    fb[i] = src[i];
  }
  
  uint32_t sync = SYNC_ADDR;
  *(uint32_t *)sync = 1;

  return len;
}

void init_device() {
  ioe_init();
}
