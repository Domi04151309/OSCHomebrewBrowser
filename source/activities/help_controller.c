#include "help_controller.h"
#include "../GRRLIB/GRRLIB.h"
#include "../res/drawables.h"
#include "../ui.h"

void HELP_CONTROLLER_render() {
  GRRLIB_DrawImg(UI_PAGE_X, 128, help_controller_img, 0, 1, 1, 0xFFFFFFFF);
}
