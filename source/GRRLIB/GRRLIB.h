#ifndef GRRLIB_COMP
#define GRRLIB_COMP

#include <grrlib.h>

void GRRLIB_InitFont();
void GRRLIB_DrawText(const int x, const int y, const char *text, unsigned int fontSize, const u32 color);
int GRRLIB_TextWidth(const char *string, unsigned int fontSize);
void GRRLIB_DrawCenteredText(const int x, const int y, const int w, const char *text, unsigned int fontSize, const u32 color);

#endif
