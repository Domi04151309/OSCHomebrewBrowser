#include "GRRLIB.h"
#include "ttf_font_ttf.h"

GRRLIB_ttfFont *font = NULL;

void GRRLIB_InitFont() {
	if (font == NULL) font = GRRLIB_LoadTTF(ttf_font_ttf, ttf_font_ttf_size);
}

void GRRLIB_DrawText(int x, int y, const char *text, unsigned int fontSize, const u32 color) {
  GRRLIB_PrintfTTF(x, y, font, text, fontSize, color);
}

int GRRLIB_TextWidth(const char *string, unsigned int fontSize) {
	return GRRLIB_WidthTTF(font, string, fontSize);
}
