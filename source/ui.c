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

void UI_roundedRect(const int x, const int y, const int w, const int h, const u32 color) {
  GRRLIB_Rectangle(x + UI_CORNER_RADIUS, y, w - 2 * UI_CORNER_RADIUS, h, color, true);
  GRRLIB_Rectangle(x, y + UI_CORNER_RADIUS, w, h - 2 * UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w - UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w -UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
}

void UI_highlight(const int x, const int y, const int w, const int h) {
  GRRLIB_Rectangle(x, y, w, h, HIGHLIGHT_COLOR, true);
}

void UI_drawTooltip(const int x, const int y, const char *string) {
  int w = 2 * UI_PADDING + GRRLIB_TextWidth(string, FONTSIZE_SMALL);
  UI_roundedRect(x - w, y, w, UI_BLOCK_BTN_H, TOOLTIP_COLOR);
  GRRLIB_DrawText(x - w + UI_PADDING, y + UI_PADDING, string, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY);
}

void UI_drawButton(const int x, const int y, const char *text, const uint8_t state) {
  u32 color;
  if (state == 1) color = BTN_COLOR_HOVER;
  else if (state == 2) color = BTN_COLOR_DISABLED;
  else color = BTN_COLOR;
  int w = 2 * UI_PADDING + GRRLIB_TextWidth(text, FONTSIZE_SMALL);

  UI_roundedRect(x, y, w, UI_BLOCK_BTN_H, color);

  GRRLIB_DrawText(x + UI_PADDING, y + UI_PADDING, text, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
}

void UI_drawBlockButton(const int x, const int y, const char *text, const uint8_t state) {
  u32 color;
  if (state == 1) color = BTN_COLOR_HOVER;
  else if (state == 2) color = BTN_COLOR_DISABLED;
  else color = BTN_COLOR;

  UI_roundedRect(x, y, UI_BLOCK_BTN_W, UI_BLOCK_BTN_H, color);
  GRRLIB_DrawText(x + (UI_BLOCK_BTN_W - GRRLIB_TextWidth(text, FONTSIZE_SMALL)) / 2, y + UI_PADDING, text, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
}

bool UI_isOnSquare(ir_t ir, const int x, const int y, const int w, const int h) {
  if (ir.x > x && ir.x < x + w && ir.y > y && ir.y < y + h) return 1;
  else return 0;
}

bool UI_isOnImg(ir_t ir, const int x, const int y, const GRRLIB_texImg *img) {
  return UI_isOnSquare(ir, x, y, (*img).w, (*img).h);
}

bool UI_isOnButton(ir_t ir, const int x, const int y, const char *text) {
  int w = 2 * UI_PADDING + GRRLIB_TextWidth(text, FONTSIZE_SMALL);
  return UI_isOnSquare(ir, x, y, w, UI_BLOCK_BTN_H);
}

bool UI_isOnBlockButton(ir_t ir, const int x, const int y) {
  return UI_isOnSquare(ir, x, y, UI_BLOCK_BTN_W, UI_BLOCK_BTN_H);
}

int UI_CAT_catTextOffset(const int index, const char *string) {
  return UI_CAT_X + UI_CAT_W * index + (UI_CAT_W - GRRLIB_TextWidth(string, FONTSIZE_SMALL)) / 2;
}

void UI_CAT_drawBar() {
  UI_roundedRect(UI_CAT_X, UI_CAT_Y, UI_CAT_W * 5, UI_BLOCK_BTN_H, BTN_COLOR);
  GRRLIB_DrawText(UI_CAT_catTextOffset(0, STR_DEMOS), UI_CAT_Y + UI_PADDING, STR_DEMOS, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
  GRRLIB_DrawText(UI_CAT_catTextOffset(1, STR_EMULATORS), UI_CAT_Y + UI_PADDING, STR_EMULATORS, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
  GRRLIB_DrawText(UI_CAT_catTextOffset(2, STR_GAMES), UI_CAT_Y + UI_PADDING, STR_GAMES, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
  GRRLIB_DrawText(UI_CAT_catTextOffset(3, STR_MEDIA), UI_CAT_Y + UI_PADDING, STR_MEDIA, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
  GRRLIB_DrawText(UI_CAT_catTextOffset(4, STR_UTILITIES), UI_CAT_Y + UI_PADDING, STR_UTILITIES, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
}

void UI_CAT_highlightHelper(const int category, const char *string) {
  GRRLIB_DrawText(UI_CAT_catTextOffset(category, string), UI_CAT_Y + UI_PADDING, string, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);
}

void UI_CAT_highlight(const int category, const u32 color) {
  if (category < 0 || category > 4) return;
  UI_roundedRect(UI_CAT_X + category * UI_CAT_W, UI_CAT_Y, UI_CAT_W, UI_BLOCK_BTN_H, color);
  switch (category) {
    case 0:
      UI_CAT_highlightHelper(category, STR_DEMOS);
      break;
    case 1:
      UI_CAT_highlightHelper(category, STR_EMULATORS);
      break;
    case 2:
      UI_CAT_highlightHelper(category, STR_GAMES);
      break;
    case 3:
      UI_CAT_highlightHelper(category, STR_MEDIA);
      break;
    case 4:
      UI_CAT_highlightHelper(category, STR_UTILITIES);
      break;
  }
}

bool UI_CAT_isOnCategory(ir_t ir, const int category) {
  if (category < 0 || category > 4) return false;
  if (UI_isOnSquare(ir, UI_CAT_X + category * UI_CAT_W, UI_CAT_Y, UI_CAT_W, UI_BLOCK_BTN_H)) {
    UI_CAT_highlight(category, BTN_COLOR_HOVER);
    return true;
  } else {
    return false;
  }
}
