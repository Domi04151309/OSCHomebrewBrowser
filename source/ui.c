#include "ui.h"

#include <wiiuse/wpad.h>
#include "GRRLIB/GRRLIB.h"

#include "beer_png.h"
#include "res.h"

void UI_bootScreen(const char *string) {
  UI_bootScreenTwo(string, " ");
}

void UI_bootScreenTwo(const char *string1, const char *string2) {
  GRRLIB_FillScreen(0x000000FF);
  GRRLIB_texImg *beer = GRRLIB_LoadTexture(beer_png);
  GRRLIB_DrawImg(512, 240 - (*beer).h / 2, beer, 0, 1, 1, 0xFFFFFFFF);
  GRRLIB_FreeTexture(beer);
  GRRLIB_DrawText(64, 240 - FONTSIZE_NORMAL - 8, "Homebrew Browser Lite", FONTSIZE_NORMAL, TEXT_COLOR_PRIMARY_DARK);
  GRRLIB_DrawText(64, 240 + 8, string1, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY_DARK);
  GRRLIB_DrawText(64, 240 + 8 + FONTSIZE_SMALL, string2, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY_DARK);
  GRRLIB_Render();
}

void UI_roundedRect(int x, int y, int w, int h, u32 color) {
  GRRLIB_Rectangle(x + UI_CORNER_RADIUS, y, w - 2 * UI_CORNER_RADIUS, h, color, true);
  GRRLIB_Rectangle(x, y + UI_CORNER_RADIUS, w, h - 2 * UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w - UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w -UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
}

void UI_drawButton(int x, int y, const char *text, uint8_t state) {
  u32 color;
  if (state == 1) color = BTN_COLOR_HOVER;
  else if (state == 2) color = BTN_COLOR_DISABLED;
  else color = BTN_COLOR;
  int w = 2 * UI_PADDING + GRRLIB_TextWidth(text, FONTSIZE_SMALL);

  UI_roundedRect(x, y, w, UI_BLOCK_BTN_H, color);

  GRRLIB_DrawText(x + UI_PADDING, y + UI_PADDING, text, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
}

int UI_isOnButton(ir_t ir, int x, int y, const char *text) {
  int w = 2 * UI_PADDING + GRRLIB_TextWidth(text, FONTSIZE_SMALL);

  if (ir.x > x && ir.x < x + w && ir.y > y && ir.y < y + UI_BLOCK_BTN_H) return 1;
  else return 0;
}

void UI_drawBlockButton(int x, int y, const char *text, uint8_t state) {
  u32 color;
  if (state == 1) color = BTN_COLOR_HOVER;
  else if (state == 2) color = BTN_COLOR_DISABLED;
  else color = BTN_COLOR;

  UI_roundedRect(x, y, UI_BLOCK_BTN_W, UI_BLOCK_BTN_H, color);
  GRRLIB_DrawText(x + UI_PADDING, y + UI_PADDING, text, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
}

int UI_isOnBlockButton(ir_t ir, int x, int y) {
  if (ir.x > x && ir.x < x + UI_BLOCK_BTN_W && ir.y > y && ir.y < y + UI_BLOCK_BTN_H) return 1;
  else return 0;
}
