#include "am.h"

#define KEYDOWN_MASK 0x8000
#define KEYCODE_MASK 0x7FFF

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  unsigned kbdreg = inl(KBD_ADDR);
  kbd->keycode = kbdreg & KEYCODE_MASK;
  kbd->keydown = kbdreg & KEYDOWN_MASK;
}
