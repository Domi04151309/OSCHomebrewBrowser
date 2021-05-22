
#ifndef GRRLIB_COMP
#define GRRLIB_COMP

#include <grrlib.h>

void GRRLIB_InitFreetype();
void GRRLIB_DrawText(int x, int y, const char *text, unsigned int fontSize, const u32 color);
GRRLIB_texImg *GRRLIB_TextToTexture(const char *string, unsigned int fontSize, unsigned int fontColour);

#endif
