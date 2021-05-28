#include "settings.h"

#include <stdio.h>
#include <stdbool.h>

#include "../GRRLIB/GRRLIB.h"
#include "../res/drawables.h"
#include "../res/res.h"
#include "../common.h"
#include "../strings.h"
#include "../utils.h"
#include "../ui.h"

uint8_t menu_section = 0;
bool select_repo = false;
bool select_category = false;
bool select_sort = false;

void SETTINGS_render() {
  GRRLIB_DrawImg(82, 146, gear_bg_img, 0, 1, 1, 0xFFFFFFFF);

  if (menu_section == 0) {
    if (UI_isOnSquare(ir, 104, 146, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 146, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_sd_card == true) setting_sd_card = false;
        else setting_sd_card = true;
      }
    } else if (UI_isOnSquare(ir, 104, 196, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 196, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_hide_installed == true) setting_hide_installed = false;
        else setting_hide_installed = true;
      }
    } else if (UI_isOnSquare(ir, 104, 246, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 246, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_rumble == true) setting_rumble = false;
        else setting_rumble = true;
      }
    } else if (UI_isOnSquare(ir, 104, 296, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 296, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_update_icon == true) setting_update_icon = false;
        else setting_update_icon = true;
      }
    } else if (UI_isOnSquare(ir, 104, 346, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 346, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_online == true) setting_online = false;
        else setting_online = true;
      }
    }

    GRRLIB_DrawText(112, 160, STR_SHOW_FREE_SPACE, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 210, STR_HIDE_INSTALLED_APPS, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 260, STR_RUMBLE, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 310, STR_UPDATE_ICONS, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 360, STR_OFFLINE_MODE, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);

    if (setting_sd_card) GRRLIB_DrawImg(498, 148, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 148, app_cross_img, 0, 1, 1, 0xFFFFFFFF);
    if (setting_hide_installed) GRRLIB_DrawImg(498, 198, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 198, app_cross_img, 0, 1, 1, 0xFFFFFFFF);
    if (setting_rumble) GRRLIB_DrawImg(498, 248, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 248, app_cross_img, 0, 1, 1, 0xFFFFFFFF);
    if (setting_update_icon) GRRLIB_DrawImg(498, 298, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 298, app_cross_img, 0, 1, 1, 0xFFFFFFFF);
    if (!setting_online) GRRLIB_DrawImg(498, 348, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 348, app_cross_img, 0, 1, 1, 0xFFFFFFFF);

    GRRLIB_DrawText(355, 418, STR_1_OF_3, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
  } else if (menu_section == 1) {
    if (select_repo == false && select_category == false && select_sort == false) {
      if (UI_isOnSquare(ir, 104, 146, 440, 44)) {
        UTILS_rumble();
        UI_highlight(104, 146, 440, 44);
        if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
          if (setting_tool_tip == true) setting_tool_tip = false;
          else setting_tool_tip = true;
        }
      } else if (UI_isOnSquare(ir, 104, 196, 440, 44)) {
        UTILS_rumble();
        UI_highlight(104, 196, 440, 44);
        if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
          if (setting_use_sd == true) setting_use_sd = false;
          else setting_use_sd = true;
        }
      } else if (UI_isOnSquare(ir, 104, 246, 440, 44)) {
        UTILS_rumble();
        UI_highlight(104, 246, 440, 44);
        if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
          select_repo = true;
          wait_a_press = 10;
        }
      } else if (UI_isOnSquare(ir, 104, 296, 440, 44)) {
        UTILS_rumble();
        UI_highlight(104, 296, 440, 44);
        if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
          select_category = true;
          wait_a_press = 10;
        }
      } else if (UI_isOnSquare(ir, 104, 346, 440, 44)) {
        UTILS_rumble();
        UI_highlight(104, 346, 440, 44);
        if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
          select_sort = true;
          wait_a_press = 10;
        }
      }
    }

    GRRLIB_DrawText(112, 160, STR_SHOW_TOOLTIPS, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 210, STR_DOWNLOAD_TO_SD_CARD, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 260, STR_CHANGE_REPOSITORY, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 310, STR_START_UP_CATEGORY, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 360, STR_START_UP_SORTING, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);

    if (setting_tool_tip) GRRLIB_DrawImg(498, 148, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 148, app_cross_img, 0, 1, 1, 0xFFFFFFFF);
    if (setting_use_sd) GRRLIB_DrawImg(498, 198, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 198, app_cross_img, 0, 1, 1, 0xFFFFFFFF);

    GRRLIB_DrawText(355, 418, STR_2_OF_3, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
  } else if (menu_section == 2) {
    if (UI_isOnSquare(ir, 104, 146, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 146, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_disusb == true) setting_disusb = false;
        else setting_disusb = true;
      }
    } else if (UI_isOnSquare(ir, 104, 196, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 196, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_wiiside == true) setting_wiiside = false;
        else setting_wiiside = true;
      }
    } else if (UI_isOnSquare(ir, 104, 246, 440, 44)) {
      UTILS_rumble();
      UI_highlight(104, 246, 440, 44);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        if (setting_server == true) setting_server = false;
        else setting_server = true;
      }
    }

    GRRLIB_DrawText(112, 160, STR_DISABLE_USB_MOUNTING, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 210, STR_USE_WIIMOTE_SIDEWAYS, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
    GRRLIB_DrawText(112, 260, STR_USE_SECONDARY_SERVER, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);

    if (setting_disusb) GRRLIB_DrawImg(498, 148, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 148, app_cross_img, 0, 1, 1, 0xFFFFFFFF);
    if (setting_wiiside) GRRLIB_DrawImg(498, 198, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 198, app_cross_img, 0, 1, 1, 0xFFFFFFFF);
    if (setting_server) GRRLIB_DrawImg(498, 248, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
    else GRRLIB_DrawImg(498, 248, app_cross_img, 0, 1, 1, 0xFFFFFFFF);

    GRRLIB_DrawText(355, 418, STR_3_OF_3, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
  }

  // Settings navigation
  if (menu_section > 0) {
    if (((pressed & WPAD_BUTTON_MINUS) || (!setting_wiiside && pressed & WPAD_BUTTON_LEFT) || (pressed_gc & PAD_TRIGGER_L) || (pressed_gc & PAD_BUTTON_LEFT)) && !select_repo && !select_category && !select_sort) {
      menu_section--;
    }
    if (UI_isOnImg(ir, 296, 400, prev_img) && !select_repo && !select_category && !select_sort) {
      UTILS_rumble();
      GRRLIB_DrawImg(296, 400, prev_img, 0, 1, 1, BTN_COLOR_HOVER);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        menu_section--;
      }
    } else {
      GRRLIB_DrawImg(296, 400, prev_img, 0, 1, 1, BTN_COLOR);
    }
  }
  if (menu_section < 2) {
    if (((pressed & WPAD_BUTTON_PLUS) || (!setting_wiiside && pressed & WPAD_BUTTON_RIGHT) || (pressed_gc & PAD_TRIGGER_R) || (pressed_gc & PAD_BUTTON_RIGHT)) && !select_repo && !select_category && !select_sort) {
      menu_section++;
    }
    if (UI_isOnImg(ir, 452, 400, next_img) && !select_repo && !select_category && !select_sort) {
      UTILS_rumble();
      GRRLIB_DrawImg(452, 400, next_img, 0, 1, 1, BTN_COLOR_HOVER);
      if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
        menu_section++;
      }
    } else {
      GRRLIB_DrawImg(452, 400, next_img, 0, 1, 1, BTN_COLOR);
    }
  }

  if ((pressed & WPAD_BUTTON_HOME || pressed_gc & PAD_BUTTON_START || pressed & WPAD_BUTTON_B || pressed_gc & PAD_BUTTON_B) && wait_a_press == 0) {
    menu_section = 0;
    update_settings();
  }
}
