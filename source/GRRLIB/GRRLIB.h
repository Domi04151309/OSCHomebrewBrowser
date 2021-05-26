
#ifndef GRRLIB_COMP
#define GRRLIB_COMP

#include <grrlib.h>

void GRRLIB_InitFont();
void GRRLIB_DrawText(int x, int y, const char *text, unsigned int fontSize, const u32 color);
int GRRLIB_TextWidth(const char *string, unsigned int fontSize);

#endif
