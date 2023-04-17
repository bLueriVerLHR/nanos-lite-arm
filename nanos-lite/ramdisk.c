#include "common.h"

char ramdisk[1024];

#define RAMDISK_SIZE ((&ramdisk[1023]) - (&ramdisk[0]))

/* The kernel is monolithic, therefore we do not need to
 * translate the address `buf' from the user process to
 * a physical one, which is necessary for a microkernel.
 */

/* read `len' bytes starting from `offset' of ramdisk into `buf' */
size_t ramdisk_read(void *buf, size_t offset, size_t len) {
  assert(offset + len <= RAMDISK_SIZE);
  memcpy(buf, ramdisk + offset, len);
  return len;
}

/* write `len' bytes starting from `buf' into the `offset' of ramdisk */
size_t ramdisk_write(const void *buf, size_t offset, size_t len) {
  assert(offset + len <= RAMDISK_SIZE);
  memcpy(ramdisk + offset, buf, len);
  return len;
}

void init_ramdisk() {
}

size_t get_ramdisk_size() {
  return RAMDISK_SIZE;
}
