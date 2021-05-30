#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <wiiuse/wpad.h>
#include <stdbool.h>

extern ir_t ir;
extern bool doRumble;
extern int pressed;
extern int wait_a_press;

void UTILS_rumble();

#endif
