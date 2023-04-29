#include <stddef.h>

#include "am.h"

#define SYNC_ADDR (VGACTL_ADDR + 4)

static int WIDTH = 0, HEIGHT = 0;

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
    .width = WIDTH, .height = HEIGHT,
    .vmemsz = WIDTH * HEIGHT * 3
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pixels = ctl->pixels;

  for (int row = 0; row < ctl->h; row++) {
    int32_t *dstp = (int32_t *)(&fb[ctl->x + (ctl->y + row) * WIDTH]);
    int32_t *srcp = (int32_t *)(pixels);
    ctl->w = ctl->w >> 2;
    size_t len = (ctl->w);
    if ((dstp != ((void*)0)) && (srcp != ((void*)0))) {
      while (len--)
        *dstp++ = *srcp++;
    }
    pixels += ctl->w;
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, true);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
