#include "menu.h"

#include <stdio.h>
#include <fat.h>
#include <wiiuse/wpad.h>
#include <unistd.h>

#include "../GRRLIB/GRRLIB.h"
#include "../res/drawables.h"
#include "../res/strings.h"
#include "../activities.h"
#include "../common.h"
#include "../utils.h"
#include "../ui.h"

void MENU_render() {
  GRRLIB_DrawImg(82, 146, home_bg_img, 0, 1, 1, 0xFFFFFFFF);

  if (UI_blockButton(ir, 283, UI_MENU_BTN_1_Y, STR_ABOUT)) {
    if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
      ACTIVITIES_open(ACTIVITY_ABOUT);
    }
  }

  if (UI_blockButton(ir, 283, UI_MENU_BTN_2_Y, STR_CONTROLLER)) {
    if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
      ACTIVITIES_open(ACTIVITY_HELP_CONTROLLER);
    }
  }

  if (UI_blockButton(ir, 283, UI_MENU_BTN_3_Y, STR_SETTINGS)) {
    if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
      ACTIVITIES_open(ACTIVITY_SETTINGS);
    }
  }

  if (UI_blockButton(ir, 283, UI_MENU_BTN_4_Y, STR_RETURN_TO_WII_MENU)) {
    if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
      WPAD_Rumble(WPAD_CHAN_0, 0);
      WPAD_Rumble(WPAD_CHAN_0, 0);
      if (download_icon > 0) {
        changing_cat = true;
        while (download_icon_sleeping != true) {
          usleep(10000);
        }
      }
      usleep(300000);
      fatUnmount("sd:");
      fatUnmount("usb:");
      WII_Initialize();
      WII_ReturnToMenu();
    }
  }

  if (UI_blockButton(ir, 283, UI_MENU_BTN_5_Y, STR_RETURN_TO_LOADER)) {
    if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
      WPAD_Rumble(WPAD_CHAN_0, 0);
      WPAD_Rumble(WPAD_CHAN_0, 0);
      usleep(300000);
      if (download_icon > 0) {
        changing_cat = true;
        while (download_icon_sleeping != true) {
          usleep(10000);
        }
      }
      fatUnmount("sd:");
      fatUnmount("usb:");
      exit(0);
    }
  }
}
