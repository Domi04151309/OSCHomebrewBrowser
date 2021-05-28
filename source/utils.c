#include "utils.h"

#include <stdio.h>
#include <wiiuse/wpad.h>
#include <stdbool.h>

ir_t ir;
bool doRumble = false;
int pressed = 0;
int pressed_gc = 0;

void UTILS_rumble() {
  doRumble = true;
}
