#ifndef _UI_H_
#define _UI_H_

#include "GRRLIB/GRRLIB.h"

#define UI_PADDING 8
#define UI_CORNER_RADIUS 4

void UI_bootScreen(char *string);
void UI_bootScreenTwo(char *string1, char *string2);
void UI_drawButton(int x, int y, char *text, uint8_t hover);
int UI_isOnButton(int irX, int irY, int x, int y);

#endif
