#ifndef _UI_H_
#define _UI_H_

#include <wiiuse/wpad.h>
#include "GRRLIB/GRRLIB.h"

#include "res.h"

#define UI_PADDING 12
#define UI_CORNER_RADIUS 4

#define UI_BLOCK_BTN_W (2 * UI_PADDING + 224)
#define UI_BLOCK_BTN_H (2 * UI_PADDING + FONTSIZE_SMALL)

void UI_bootScreen(const char *string);
void UI_bootScreenTwo(const char *string1, const char *string2);
void UI_roundedRect(int x, int y, int w, int h, u32 color);
int UI_isOnImg(ir_t ir, int x, int y, const GRRLIB_texImg *img);
void UI_drawButton(int x, int y, const char *text, uint8_t state);
int UI_isOnButton(ir_t ir, int x, int y, const char * text);
void UI_drawBlockButton(int x, int y, const char *text, uint8_t state);
int UI_isOnBlockButton(ir_t ir, int x, int y);

#endif
