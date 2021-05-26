#ifndef _UI_H_
#define _UI_H_

#include <wiiuse/wpad.h>
#include "GRRLIB/GRRLIB.h"

#include "common.h"
#include "res.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT xfb_height

#define UI_PADDING 12
#define UI_CORNER_RADIUS 4

#define UI_BLOCK_BTN_W (2 * UI_PADDING + 224)
#define UI_BLOCK_BTN_H (2 * UI_PADDING + FONTSIZE_SMALL)

#define UI_PAGE_X 32
#define UI_PAGE_Y UI_CAT_Y
#define UI_PAGE_W_SMALL (SCREEN_WIDTH - 32 - 8 - 36 - 8 - UI_PAGE_X)
#define UI_PAGE_W (SCREEN_WIDTH - 32 - UI_PAGE_X)
#define UI_PAGE_H (SCREEN_HEIGHT - 32 - UI_PAGE_Y)

#define UI_SORT_X (SCREEN_WIDTH - 32 - 36 - 8)
#define UI_SORT_ARROW_X (SCREEN_WIDTH - 32 - 12)
#define UI_SORT_1_Y (UI_PAGE_Y + UI_BLOCK_BTN_H + UI_PADDING)
#define UI_SORT_2_Y (UI_PAGE_Y + UI_BLOCK_BTN_H + UI_PADDING + 36 + 16)
#define UI_SORT_3_Y (SCREEN_HEIGHT - 32 - UI_PADDING - 36)

#define UI_CAT_X 32
#define UI_CAT_Y (16 + 72 + 8)
#define UI_CAT_W (UI_PAGE_W / 5)

void UI_bootScreen(const char *string);
void UI_bootScreenTwo(const char *string1, const char *string2);
void UI_roundedRect(const f32 x, const f32 y, const f32 w, const f32 h, const u32 color);
void UI_highlight(const f32 x, const f32 y, const f32 w, const f32 h);
void UI_drawTooltip(const f32 x, const f32 y, const char *string);
void UI_drawButton(const f32 x, const f32 y, const char *text, const uint8_t state);
void UI_drawBlockButton(const f32 x, const f32 y, const char *text, const uint8_t state);
bool UI_isOnSquare(ir_t ir, const f32 x, const f32 y, const f32 w, const f32 h);
bool UI_isOnImg(ir_t ir, const f32 x, const f32 y, const GRRLIB_texImg *img);
bool UI_isOnButton(ir_t ir, const f32 x, const f32 y, const char * text);
bool UI_isOnBlockButton(ir_t ir, const f32 x, const f32 y);

int UI_CAT_catTextOffset(const int index, const char *string);
void UI_CAT_drawBar();
void UI_CAT_highlightHelper(const int category, const char *string);
void UI_CAT_highlight(const int category, const u32 color);
bool UI_CAT_isOnCategory(ir_t ir, const int category);

#endif
