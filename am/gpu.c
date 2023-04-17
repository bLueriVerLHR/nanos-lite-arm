#include <stddef.h>

#include "am.h"

#define SYNC_ADDR (VGACTL_ADDR + 4)

static int WIDTH = 0, HEIGHT = 0;

#define fbdraw(TYPE, k, dst, src, n) \
TYPE *dstp = (TYPE *)(dst); \
TYPE *srcp = (TYPE *)(src); \
n = n >> k;\
size_t len = (n); \
if ((dstp != NULL) && (srcp != NULL)) { \
  while (len--) *dstp++ = *srcp++; \
}


void __am_gpu_init() {
  WIDTH  = inw(VGACTL_ADDR + 2);
  HEIGHT = inw(VGACTL_ADDR);
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (int i = 0; i < WIDTH * HEIGHT; ++i) fb[i] = i;
  outl(SYNC_ADDR, true);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = 400, .height = 300,
    .vmemsz = 400 * 300 * 3
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pixels = ctl->pixels;

  for (int row = 0; row < ctl->h; row++) {
    fbdraw(int32_t, 2, &fb[ctl->x + (ctl->y + row) * WIDTH], pixels, ctl->w);
    pixels += ctl->w;
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, true);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
