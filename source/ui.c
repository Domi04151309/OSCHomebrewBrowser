#include "ui.h"
#include "beer_png.h"
#include "GRRLIB/GRRLIB.h"
#include "res.h"

void UI_bootScreen(char *string) {
  UI_bootScreenTwo(string, " ");
}

void UI_bootScreenTwo(char *string1, char *string2) {
  GRRLIB_FillScreen(0x000000FF);
  GRRLIB_texImg *beer = GRRLIB_LoadTexture(beer_png);
  GRRLIB_DrawImg(512, 240 - (*beer).h / 2, beer, 0, 1, 1, 0xFFFFFFFF);
  GRRLIB_FreeTexture(beer);
  GRRLIB_DrawText(64, 240 - FONTSIZE_NORMAL - 8, "Homebrew Browser Lite", FONTSIZE_NORMAL, TEXT_COLOR_PRIMARY_DARK);
  GRRLIB_DrawText(64, 240 + 8, string1, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY_DARK);
  GRRLIB_DrawText(64, 240 + 8 + FONTSIZE_SMALL, string2, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY_DARK);
  GRRLIB_Render();
}

void UI_drawButton(int x, int y, char *text, uint8_t hover) {
  u32 color;
  if (hover == 0) color = BTN_COLOR;
  else color = BTN_COLOR_HOVER;
  int w = 2 * UI_PADDING + 256;
  int h = 2 * UI_PADDING + FONTSIZE_NORMAL;

  GRRLIB_Rectangle(x + UI_CORNER_RADIUS, y, w - 2 * UI_CORNER_RADIUS, h, color, true);
  GRRLIB_Rectangle(x, y + UI_CORNER_RADIUS, w, h - 2 * UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w - UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w -UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);

  GRRLIB_DrawText(x + UI_PADDING, y + UI_PADDING, text, FONTSIZE_NORMAL, TEXT_COLOR_PRIMARY_DARK);
}

int UI_isOnButton(int irX, int irY, int x, int y) {
  int w = 2 * UI_PADDING + 256;
  int h = 2 * UI_PADDING + FONTSIZE_NORMAL;

  if (irX > x && irX < x + w && irY > y && irY < y + h ) return 1;
  else return 0;
}
