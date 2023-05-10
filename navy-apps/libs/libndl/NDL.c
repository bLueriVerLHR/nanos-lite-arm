#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;


static uint32_t NDL_INIT_TIME;
static struct timeval time_val;

uint32_t NDL_GetTicks() {
  gettimeofday(&time_val, NULL);
  return time_val.tv_sec * 1000 + time_val.tv_usec / 1000 - NDL_INIT_TIME;
}

int NDL_PollEvent(char *buf, int len) {
  int fp = open("/dev/events", O_RDONLY, 0);
  size_t cnt = read(fp, buf, len);
  return !!cnt;
}

#define W 400
#define H 300
#define CSIZE W * H * 4

static int canvas_w = W, canvas_h = H,
           canvas_x = 0, canvas_y = 0;

void NDL_OpenCanvas(int *w, int *h) {
  if (*w == 0) {
    *w = screen_w;
  }
  if (*h == 0) {
    *h = screen_h;
  }
  if (*h > screen_h || *w > screen_w) {
    assert(0 && "Screen Size Exceeded!");
  }
  canvas_w = *w;
  canvas_h = *h;
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  int graphics = open("/dev/fb", O_RDWR);
  for (int i = 0; i < h; ++i) {
    int tw = (canvas_y + y + i) * screen_w;
    int tx = canvas_x + x;
    lseek(graphics, (tw + tx) * sizeof(uint32_t), SEEK_SET);
    size_t s = write(graphics, pixels + w * i, w * sizeof(uint32_t));
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  gettimeofday(&time_val, NULL);
  NDL_INIT_TIME = time_val.tv_usec;
  char info[128];
  int dispinfo = open("/proc/dispinfo", 0);
  read(dispinfo, info, sizeof(info));
  // printf("%s\n", info);
  sscanf(info, "%d,%d", &screen_w, &screen_h);
  printf("w = %d, h = %d\n", screen_w, screen_h);
  return 0;
}

void NDL_Quit() {
}
