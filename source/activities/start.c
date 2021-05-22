#include "start.h"
#include "beer_png.h"
#include "../GRRLIB/GRRLIB.h"
#include "../res.h"

void START_new() {
  GRRLIB_FillScreen(0x000000FF);
  GRRLIB_texImg *beer = GRRLIB_LoadTexture(beer_png);
  GRRLIB_DrawImg(512, 240 - (*beer).h / 2, beer, 0, 1, 1, 0xFFFFFFFF);
  free(beer);
  GRRLIB_DrawText(64, 240 - FONTSIZE_NORMAL - 8, "Homebrew Browser Lite", FONTSIZE_NORMAL, TEXT_COLOUR_PRIMARY_DARK);
}

void START_showTextTwo(char *string1, char *string2) {
  START_new();
  GRRLIB_DrawText(64, 240 + 8, string1, FONTSIZE_SMALL, TEXT_COLOUR_SECONDARY_DARK);
  GRRLIB_DrawText(64, 240 + 8 + FONTSIZE_SMALL, string2, FONTSIZE_SMALL, TEXT_COLOUR_SECONDARY_DARK);
  GRRLIB_Render();
}

void START_showText(char *string) {
  START_new();
  GRRLIB_DrawText(64, 240, string, FONTSIZE_SMALL, TEXT_COLOUR_SECONDARY_DARK);
  GRRLIB_Render();
}
