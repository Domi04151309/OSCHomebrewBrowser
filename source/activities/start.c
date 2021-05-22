#include "start.h"
#include "beer_png.h"
#include "../GRRLIB/GRRLIB.h"
#include "../res.h"

void START_new() {
  GRRLIB_FillScreen(0xFF000000);

  GRRLIB_texImg *appName = GRRLIB_TextToTexture("Homebrew Browser Lite", FONTSIZE_NORMAL, TEXT_COLOUR_PRIMARY_DARK);
  GRRLIB_DrawImg(64, 240 - (*appName).h, appName, 0, 1, 1, 0xFFFFFFFF);
  free(appName);

  GRRLIB_texImg *beer = GRRLIB_LoadTexture(beer_png);
  GRRLIB_DrawImg(512, 240 - (*beer).h / 2, beer, 0, 1, 1, 0xFFFFFFFF);
  free(beer);
}

void START_showTextTwo(char *string1, char *string2) {
  START_new();
  GRRLIB_texImg *stringTex = calloc(1, sizeof(GRRLIB_texImg));
  stringTex = GRRLIB_TextToTexture(string1, FONTSIZE_SMALL, TEXT_COLOUR_SECONDARY_DARK);
  GRRLIB_DrawImg(64, 240, stringTex, 0, 1, 1, 0xFFFFFFFF);
  /*stringTex = GRRLIB_TextToTexture(string2, FONTSIZE_SMALL, TEXT_COLOUR_SECONDARY_DARK);
  GRRLIB_DrawImg(64, 240 + (*stringTex).h, stringTex, 0, 1, 1, 0xFFFFFFFF);*/
  free(stringTex);
  GRRLIB_Render();
}

void START_showText(char *string) {
  START_new();
  GRRLIB_texImg *stringTex = GRRLIB_TextToTexture(string, FONTSIZE_SMALL, TEXT_COLOUR_SECONDARY_DARK);
  GRRLIB_DrawImg(64, 240, stringTex, 0, 1, 1, 0xFFFFFFFF);
  free(stringTex);
  GRRLIB_Render();
}
