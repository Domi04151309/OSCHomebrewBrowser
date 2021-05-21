#include "start.h"
#include "../GRRLIB/GRRLIB.h"
#include "../res.h"

void START_showText(char *text) {
  GRRLIB_texImg *appName = GRRLIB_TextToTexture("Homebrew Browser Lite", FONTSIZE_NORMAL, TEXT_COLOUR_PRIMARY_DARK);
  GRRLIB_texImg *texImg = GRRLIB_TextToTexture(text, FONTSIZE_SMALL, TEXT_COLOUR_SECONDARY_DARK);
  GRRLIB_FillScreen(0xFF000000);
  GRRLIB_DrawImg(64, 240 - (*appName).h, appName, 0, 1, 1, 0xFFFFFFFF);
  GRRLIB_DrawImg(64, 240 + (*texImg).h, texImg, 0, 1, 1, 0xFFFFFFFF);
  GRRLIB_Render();
  free(appName);
  free(texImg);
}
