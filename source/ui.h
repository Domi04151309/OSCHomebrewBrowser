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
void UI_roundedRect(const int x, const int y, const int w, const int h, const u32 color);
void UI_highlight(const int x, const int y, const int w, const int h);
void UI_drawTooltip(const int x, const int y, const char *string);
void UI_drawButton(const int x, const int y, const char *text, const uint8_t state);
void UI_drawBlockButton(const int x, const int y, const char *text, const uint8_t state);
bool UI_isOnSquare(ir_t ir, const int x, const int y, const int w, const int h);
bool UI_isOnImg(ir_t ir, const int x, const int y, const GRRLIB_texImg *img);
bool UI_isOnButton(ir_t ir, const int x, const int y, const char * text);
bool UI_isOnBlockButton(ir_t ir, const int x, const int y);

#define UI_CAT_X 35
#define UI_CAT_Y 96
#define UI_CAT_W (530 / 5)

int UI_CAT_catTextOffset(const int index, const char *string);
void UI_CAT_drawBar();
void UI_CAT_highlightHelper(const int category, const char *string);
void UI_CAT_highlight(const int category, const u32 color);
bool UI_CAT_isOnCategory(ir_t ir, const int category);

#endif
