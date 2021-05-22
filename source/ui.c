#include "ui.h"
#include "GRRLIB/GRRLIB.h"
#include "res.h"

void UI_drawButton(int x, int y, GRRLIB_texImg *text, uint8_t hover) {
  u32 color;
  if (hover == 0) color = BTN_COLOR;
  else color = BTN_COLOR_HOVER;
  int w = 2 * UI_PADDING + 256;
  int h = 2 * UI_PADDING + 22;

  GRRLIB_Rectangle(x + UI_CORNER_RADIUS, y, w - 2 * UI_CORNER_RADIUS, h, color, true);
  GRRLIB_Rectangle(x, y + UI_CORNER_RADIUS, w, h - 2 * UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w - UI_CORNER_RADIUS, y + UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);
  GRRLIB_Circle(x + w -UI_CORNER_RADIUS, y + h - UI_CORNER_RADIUS, UI_CORNER_RADIUS, color, true);

  GRRLIB_DrawImg(x + UI_PADDING, y + UI_PADDING, text, 0, 1, 1, 0xFFFFFFFF);
}

int UI_isOnButton(int irX, int irY, int x, int y) {
  int w = 2 * UI_PADDING + 256;
  int h = 2 * UI_PADDING + 22;

  if (irX > x && irX < x + w && irY > y && irY < y + h ) return 1;
  else return 0;
}
