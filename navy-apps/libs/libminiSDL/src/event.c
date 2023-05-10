#include <NDL.h>
#include <SDL.h>
#include <assert.h>
#include <string.h>

#define keyname(k) #k,

typedef struct __event_element{
  uint8_t type;
  uint8_t sym;
  struct __event_element* next;
} event_element;

static event_element event_queue = {.type = 0, .sym = 0, .next = NULL};

static event_element *end = &event_queue;

static void append(uint8_t type, uint8_t sym) {
  event_element *new_element = (event_element *)malloc(sizeof(event_element));
  new_element->type = type;
  new_element->sym = sym;
  new_element->next = NULL;
  end->next = new_element;
  end = new_element;
}

static int pop(uint8_t *type, uint8_t *sym) {
  if (event_queue.next == NULL) {
    return 0;
  } else {
    event_element *buf = event_queue.next;
    *type = buf->type;
    *sym = buf->sym;
    event_queue.next = buf->next;
    if (buf == end){
      end = &event_queue;
    }
    free (buf);
  }
  return 1;
}

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

static uint8_t key_state[sizeof(keyname) / sizeof(keyname[0])] = {0};

static int inline read_keyinfo(uint8_t *type, uint8_t *sym) {
  char key_buf[64];
  int ret = NDL_PollEvent(key_buf, sizeof(key_buf));
  if (!ret) return 0;

  char action = key_buf[1];
  char *key = &key_buf[3];

  *type = action == 'u' ? SDL_KEYUP : SDL_KEYDOWN;

  for (int i = 0; i < sizeof(keyname) / sizeof(char *); ++i) {
    if (key[0] == keyname[i][0] && strcmp(key, keyname[i]) == 0) {
      *sym = i;
      break;
    }
  }
  return ret;
}

int SDL_PushEvent(SDL_Event *ev) {
  assert(0 && "Not implemented");
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  uint8_t type = 0, sym = 0;
  if (read_keyinfo(&type, &sym)) {
    ev->type = type;
    ev->key.keysym.sym = sym;
    switch(type) {
    case SDL_KEYDOWN: key_state[sym] = 1; break;
    case SDL_KEYUP:   key_state[sym] = 0; break;
    }
    return 1;
  }
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  uint8_t type = 0, sym = 0;
  while (!read_keyinfo(&type, &sym));
  event->type = type;
  event->key.keysym.sym = sym;
  switch(type){
  case SDL_KEYDOWN: key_state[sym] = 1; break;
  case SDL_KEYUP:   key_state[sym] = 0; break;
  }
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  assert(0 && "Not implemented");
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  if (numkeys)
    *numkeys = sizeof(key_state) / sizeof(key_state[0]);
  return key_state;
}
