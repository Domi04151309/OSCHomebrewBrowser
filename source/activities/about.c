#include "about.h"
#include "../GRRLIB/GRRLIB.h"
#include "../res/drawables.h"
#include "../ui.h"

void ABOUT_render() {
  GRRLIB_DrawImg(UI_PAGE_X, 128, help_about_img, 0, 1, 1, 0xFFFFFFFF);
}
