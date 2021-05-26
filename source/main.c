/*

Homebrew Browser -- a simple way to view and install Homebrew releases via the Wii
Version: 0.3.9e

Author: teknecal
Created: 24/06/2008
Last Modified: 31/10/2010

Using some source from ftpii v0.0.5
ftpii Source Code Copyright (C) 2008 Joseph Jordan <joe.ftpii@psychlaw.com.au>

*/
#include <errno.h>
#include <malloc.h>
#include <math.h>
#include <network.h>
#include <ogcsys.h>
#include <gccore.h>
#include <ogc/pad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <time.h>
#include <wiiuse/wpad.h>
#include <unistd.h>
#include <fat.h>
#include <zlib.h>
#include "unzip/unzip.h"
#include "unzip/miniunz.h"
#include "common.h"
#include "GRRLIB/GRRLIB.h"
#include <dirent.h>
#include <sys/statvfs.h>

#include "mouse_png.h"
#include "control_wiimote_png.h"
#include "control_wiimote_2_png.h"
#include "control_wiimote_3_png.h"
#include "control_wiimote_4_png.h"
#include "control_nunchuck_png.h"
#include "control_classic_png.h"
#include "control_gcn_png.h"
#include "control_keyboard_png.h"
#include "control_zapper_png.h"
#include "logo_png.h"
#include "no_image_png.h"
#include "date_png.h"
#include "app_question_png.h"
#include "app_tick_png.h"
#include "app_tick_small_png.h"
#include "stack_png.h"
#include "run_png.h"
#include "app_new_png.h"
#include "list_png.h"
#include "download_png.h"
#include "blank_png.h"
#include "sort_arrow_down_png.h"
#include "sort_arrow_up_png.h"
#include "help_about_png.h"
#include "help_controller_png.h"
#include "home_bg_png.h"
#include "gear_bg_png.h"
#include "app_cross_png.h"
#include "cancel_download_prompt_png.h"
#include "button_no_png.h"
#include "button_no_highlight_png.h"
#include "button_yes_png.h"
#include "button_yes_highlight_png.h"
#include "updated_close_png.h"
#include "updated_close_highlight_png.h"
#include "name_png.h"
#include "apps_repo_png.h"
#include "apps_start_cat_png.h"
#include "apps_start_sort_png.h"
#include "next_png.h"
#include "prev_png.h"

#include "activities.h"
#include "ui.h"
#include "res.h"
#include "strings.h"

#define METHOD_SD 1
#define METHOD_USB 2

extern Mtx GXmodelView2D;

// Wiimote IR
ir_t ir;
orient_t orient;

float ir_x = 0;
float ir_y = 0;
float ir_pitch = 0;
float ir_rotation = 0;

// Time info
long current_time;
extern struct tm * timeinfo;
extern time_t app_time;

int category_selection = 2;
bool update_about = false;
bool free_update = false;
bool free_sd_size = false;
bool updated_cat = false;
int current_app = 0;

char text_white[10] = " ";
int display_message_counter = 0;

int wait_a_press = 0;
int held_counter = 0;
int menu_section = 0;
bool free_icon_info = false;
bool gc_control_used = false;
int cancel_wait = 0;
bool load_updated = false;
int icons_loaded = 0;

bool free_download_info = false;
bool select_repo = false;
bool select_category = false;
bool select_sort = false;
bool repo_texted = false;

int refresh_list = -1;
int download_arrow = 0;

s8 HWButton = -1;
void WiimotePowerPressed(s32 chan) {
	if (chan == 0) HWButton = SYS_POWEROFF_STANDBY;
}

bool close_windows() {
	bool state = select_repo || select_category || select_sort;
	select_repo = false;
	select_category = false;
	select_sort = false;
	if (ACTIVITIES_current() == ACTIVITY_SETTINGS) update_settings();
	menu_section = 0;
	refresh_list = -1;
	current_app = 0;
	return state;
}

int main(int argc, char **argv) {
	initialise();
	initialise_reset_button();
	WPAD_SetPowerButtonCallback(WiimotePowerPressed);

	current_time = time(0);
	sprintf(setting_last_boot, "%li", current_time); // bug fix

	GRRLIB_Init();
	GRRLIB_InitFreetype();
	GRRLIB_InitFont();
	UI_bootScreen("Loading");

	u32 temp_esid;
	ES_GetDeviceID(&temp_esid);
	sprintf(esid, "%d", temp_esid);
	if (esid <= 0) { printf("ESID error - You won't be able to rate applications.\n"); }

	if (setting_online) initialise_network();

	initialise_fat();
	load_settings();

	if (setting_online && !setting_server) {
		initialise_codemii();
		printf("Attempting to connect to server... ");
		int main_retries = 0;
		while (!www_passed && main_retries < 3) {
			initialise_www();
			int retries = 0;
			while (!www_passed && retries < 5) {
				sleep(1);
				retries++;
			}
			if (!www_passed) {
				printf("Failed, retrying... \n");
			}
			main_retries++;
			suspend_www_thread();
		}

		if (!www_passed) {
			codemii_backup = true;
			printf("\nOSCWii appears to be having issues, using OSCWii Backup Server.\n\n");
			initialise_codemii_backup();
			printf("Attempting to connect to server... ");

			int main_retries = 0;
			while (!www_passed && main_retries < 3) {
				initialise_www();
				int retries = 0;
				while (!www_passed && retries < 5) {
					sleep(1);
					retries++;
				}
				if (!www_passed) {
					printf("Failed, retrying... \n");
				}
				main_retries++;
				suspend_www_thread();
			}
		}

		suspend_www_thread();

		if (!www_passed) {
			die("\nReturning you back to HBC. Please check to see if " MAIN_DOMAIN " and " FALLBACK_DOMAIN " are working.");
		}
	}
	else if (setting_server) { // Secondary server setting enabled
		codemii_backup = true;
		initialise_codemii_backup();
		printf("Attempting to connect to OSCWii Secondary server... ");

		int main_retries = 0;
		while (!www_passed && main_retries < 3) {
			initialise_www();
			int retries = 0;
			while (!www_passed && retries < 5) {
				sleep(1);
				retries++;
			}
			if (!www_passed) {
				printf("Failed, retrying... \n");
			}
			main_retries++;
			suspend_www_thread();
		}

		suspend_www_thread();

		if (!www_passed) {
			die("\nReturning you back to HBC. Please check to see if " FALLBACK_DOMAIN " is working.\n");
		}
	}
	printf("Connection established\n");
	repo_check();

	// Grab the homebrew list and parse list
	while (!request_list());

	// Clear homebrew list
	clear_list();

	// List
	int set_cat = setting_category; // So we don't mess with the user's settings
	while (array_length(homebrew_list) == 0) {
		if (set_cat == 0) {
			int i;
			for (i = 0; i < array_length(demos_list); i++) {
				homebrew_list[i] = demos_list[i];
			}
			category_selection = 0;
			category_old_selection = 0;
		} else if (set_cat == 1) {
			int i;
			for (i = 0; i < array_length(emulators_list); i++) {
				homebrew_list[i] = emulators_list[i];
			}
			category_selection = 1;
			category_old_selection = 1;
		} else if (set_cat == 2) {
			int i;
			for (i = 0; i < array_length(games_list); i++) {
				homebrew_list[i] = games_list[i];
			}
			category_selection = 2;
			category_old_selection = 2;
		} else if (set_cat == 3) {
			int i;
			for (i = 0; i < array_length(media_list); i++) {
				homebrew_list[i] = media_list[i];
			}
			category_selection = 3;
			category_old_selection = 3;
		} else if (set_cat == 4) {
			int i;
			for (i = 0; i < array_length(utilities_list); i++) {
				homebrew_list[i] = utilities_list[i];
			}
			category_selection = 4;
			category_old_selection = 4;
		}
		if (array_length(homebrew_list) == 0) {
			if (set_cat == 4) {
				set_cat = 0;
			} else {
				set_cat++;
			}
		}
	}

	// Sort
	if (setting_sort == 0) { sort_by_name(0); }
	else if (setting_sort == 1) { sort_by_date(0); }

	if (setting_hide_installed) {
		hide_apps_installed();
	}

	if (setting_online && setting_repo == 0) {
		check_missing_files();
		check_temp_files();
		update_settings(); // Update last boot time
	}

	suspend_reset_thread();

	// App listing text
	GRRLIB_texImg *str_name = NULL; GRRLIB_texImg *str_name1 = NULL; GRRLIB_texImg *str_name2 = NULL; GRRLIB_texImg *str_name3 = NULL; GRRLIB_texImg *str_name4 = NULL;
	GRRLIB_texImg *str_short_desc = NULL; GRRLIB_texImg *str_short_desc1 = NULL; GRRLIB_texImg *str_short_desc2 = NULL; GRRLIB_texImg *str_short_desc3 = NULL; GRRLIB_texImg *str_short_desc4 = NULL;

	// About text
	GRRLIB_texImg *str_res_title = NULL;
	GRRLIB_texImg *str_res_author = NULL;
	GRRLIB_texImg *str_res_version = NULL;
	GRRLIB_texImg *str_res_size = NULL;
	GRRLIB_texImg *str_res_date = NULL;

	// About text description
	GRRLIB_texImg *string1 = NULL;
	GRRLIB_texImg *string2 = NULL;
	GRRLIB_texImg *string3 = NULL;
	GRRLIB_texImg *string4 = NULL;
	GRRLIB_texImg *string5 = NULL;

	GRRLIB_texImg *str_deleting = GRRLIB_TextToTexture("Deleting ...", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_download_zip_failed = GRRLIB_TextToTexture("Downloading zip file failed.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_create_folder_failed = GRRLIB_TextToTexture("Creating folders failed.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_download_icon_failed = GRRLIB_TextToTexture("Downloading icon file failed.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_extract_zip_failed = GRRLIB_TextToTexture("Extracting zip file failed.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_boot_file_failed = GRRLIB_TextToTexture("Problem checking boot.dol/elf file.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_delete_file_failed = GRRLIB_TextToTexture("Deleting boot/meta/icon files failed.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_delete_folder_failed = GRRLIB_TextToTexture("Deleting folders failed.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_delete_app_failed = GRRLIB_TextToTexture("Deleting application folder failed.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_free_space_failed = GRRLIB_TextToTexture("Not enough free space.", FONTSIZE_SMALL, 0x575757);
	GRRLIB_texImg *str_no_wifi = GRRLIB_TextToTexture("Unable to initialise network.", FONTSIZE_SMALL, 0x575757);

	GRRLIB_texImg *str_sd_card = NULL;
	GRRLIB_texImg *str_icon_info = NULL;
	GRRLIB_texImg *str_download_info = NULL;

	// Download queue
	GRRLIB_texImg *str_title_status = NULL;
	GRRLIB_texImg *str_overall_progress = GRRLIB_TextToTexture("Overall Progress", FONTSIZE_SMALL, 0x575757);

	// Settings
	GRRLIB_texImg *str_cat_1 = GRRLIB_TextToTexture("Demos", FONTSIZE_SMALL1, 0x575757);
	GRRLIB_texImg *str_cat_2 = GRRLIB_TextToTexture("Emulators", FONTSIZE_SMALL1, 0x575757);
	GRRLIB_texImg *str_cat_3 = GRRLIB_TextToTexture("Games", FONTSIZE_SMALL1, 0x575757);
	GRRLIB_texImg *str_cat_4 = GRRLIB_TextToTexture("Media", FONTSIZE_SMALL1, 0x575757);
	GRRLIB_texImg *str_cat_5 = GRRLIB_TextToTexture("Utilities", FONTSIZE_SMALL1, 0x575757);

	GRRLIB_texImg *str_sort_1 = GRRLIB_TextToTexture("Name", FONTSIZE_SMALL1, 0x575757);
	GRRLIB_texImg *str_sort_2 = GRRLIB_TextToTexture("Date", FONTSIZE_SMALL1, 0x575757);

	GRRLIB_texImg *mouse_img=GRRLIB_LoadTexture(mouse_png);

	GRRLIB_texImg *control_wiimote_img=GRRLIB_LoadTexture(control_wiimote_png);
	GRRLIB_texImg *control_wiimote_2_img=GRRLIB_LoadTexture(control_wiimote_2_png);
	GRRLIB_texImg *control_wiimote_3_img=GRRLIB_LoadTexture(control_wiimote_3_png);
	GRRLIB_texImg *control_wiimote_4_img=GRRLIB_LoadTexture(control_wiimote_4_png);
	GRRLIB_texImg *control_nunchuck_img=GRRLIB_LoadTexture(control_nunchuck_png);
	GRRLIB_texImg *control_classic_img=GRRLIB_LoadTexture(control_classic_png);
	GRRLIB_texImg *control_gcn_img=GRRLIB_LoadTexture(control_gcn_png);
	GRRLIB_texImg *control_keyboard_img=GRRLIB_LoadTexture(control_keyboard_png);
	GRRLIB_texImg *control_zapper_img=GRRLIB_LoadTexture(control_zapper_png);
	GRRLIB_texImg *logo_img=GRRLIB_LoadTexture(logo_png);
	GRRLIB_texImg *date_img=GRRLIB_LoadTexture(date_png);
	GRRLIB_texImg *name_img=GRRLIB_LoadTexture(name_png);
	GRRLIB_texImg *stack_img=GRRLIB_LoadTexture(stack_png);
	GRRLIB_texImg *run_img=GRRLIB_LoadTexture(run_png);
	GRRLIB_texImg *app_question_img=GRRLIB_LoadTexture(app_question_png);
	GRRLIB_texImg *app_tick_img=GRRLIB_LoadTexture(app_tick_png);
	GRRLIB_texImg *app_tick_small_img=GRRLIB_LoadTexture(app_tick_small_png);
	GRRLIB_texImg *app_new_img=GRRLIB_LoadTexture(app_new_png);
	GRRLIB_texImg *list_img=GRRLIB_LoadTexture(list_png);
	GRRLIB_texImg *download_img=GRRLIB_LoadTexture(download_png);
	GRRLIB_texImg *sort_arrow_down_img=GRRLIB_LoadTexture(sort_arrow_down_png);
	GRRLIB_texImg *sort_arrow_up_img=GRRLIB_LoadTexture(sort_arrow_up_png);

	GRRLIB_texImg *help_about_img=GRRLIB_LoadTexture(help_about_png);
	GRRLIB_texImg *help_controller_img=GRRLIB_LoadTexture(help_controller_png);

	GRRLIB_texImg *gear_bg_img=GRRLIB_LoadTexture(gear_bg_png);
	GRRLIB_texImg *home_bg_img=GRRLIB_LoadTexture(home_bg_png);
	GRRLIB_texImg *app_cross_img=GRRLIB_LoadTexture(app_cross_png);

	GRRLIB_texImg *cancel_download_prompt_img=GRRLIB_LoadTexture(cancel_download_prompt_png);
	GRRLIB_texImg *button_no_img=GRRLIB_LoadTexture(button_no_png);
	GRRLIB_texImg *button_no_highlight_img=GRRLIB_LoadTexture(button_no_highlight_png);
	GRRLIB_texImg *button_yes_img=GRRLIB_LoadTexture(button_yes_png);
	GRRLIB_texImg *button_yes_highlight_img=GRRLIB_LoadTexture(button_yes_highlight_png);
	GRRLIB_texImg *updated_close_img=GRRLIB_LoadTexture(updated_close_png);
	GRRLIB_texImg *updated_close_highlight_img=GRRLIB_LoadTexture(updated_close_highlight_png);
	GRRLIB_texImg *apps_repo_img=GRRLIB_LoadTexture(apps_repo_png);
	GRRLIB_texImg *apps_start_cat_img=GRRLIB_LoadTexture(apps_start_cat_png);
	GRRLIB_texImg *apps_start_sort_img=GRRLIB_LoadTexture(apps_start_sort_png);
	GRRLIB_texImg *next_img=GRRLIB_LoadTexture(next_png);
	GRRLIB_texImg *prev_img=GRRLIB_LoadTexture(prev_png);

	GRRLIB_texImg *icon1_img = NULL;
	GRRLIB_texImg *icon2_img = NULL;
	GRRLIB_texImg *icon3_img = NULL;
	GRRLIB_texImg *icon4_img = NULL;
	GRRLIB_texImg *icon5_img = NULL;

	// Load icons
	load_icons();

	// Debug
	int xx = 120;
	int yy = 375;

	int ypos = 142;
	int rumble_count = 0;
	bool doRumble = false;
	bool free_string = false;
	int string_count = 6;
	int start_updated = -1;
	int finish_updated = 0;
	int ypos_updated = 184;

	// Main Thread
	ACTIVITIES_open(ACTIVITY_MAIN);
	while(1) {

		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);
		u32 held = WPAD_ButtonsHeld(0);

		PAD_ScanPads();
		u32 pressed_gc = PAD_ButtonsDown(0);
		u32 held_gc = PAD_ButtonsHeld(0);

		int old_irx = ir.x;
		int old_iry = ir.y;

		// IR Movement
		WPAD_IR(0, &ir);
		WPAD_Orientation(0, &orient);

		if (!ir.valid) {
			ir.x = old_irx;
			ir.y = old_iry;
			ir.angle = 0;
		}

		// PAD Movement
		if (PAD_StickY(0) > 18 && ir.y > 20) {
			ir.y -=5 ;
			gc_control_used = true;
		}
		if (PAD_StickY(0) < -18 && ir.y < 455) {
			ir.y+=5;
			gc_control_used = true;
		}
		if (PAD_StickX(0) > 18 && ir.x < 625) {
			ir.x+=5;
			gc_control_used = true;
		}
		if (PAD_StickX(0) < -18 && ir.x > -10) {
			ir.x-=5;
			gc_control_used = true;
		}

		if (setting_wiiside && !(held & WPAD_BUTTON_1)) {
			if (held & WPAD_BUTTON_RIGHT) {
				ir.y -=5 ;
				gc_control_used = true;
			}
			if (held & WPAD_BUTTON_LEFT) {
				ir.y+=5;
				gc_control_used = true;
			}
			if (held & WPAD_BUTTON_DOWN) {
				ir.x+=5;
				gc_control_used = true;
			}
			if (held & WPAD_BUTTON_UP) {
				ir.x-=5;
				gc_control_used = true;
			}
		}

		GRRLIB_FillScreen(WINDOW_BACKGROUND);
		UI_roundedRect(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W, UI_PAGE_H, RES_COLOR_WHITE);

		int x;
		int start = -1;
		int finish = 0;
		for (x = 0; x < array_length(homebrew_list); x++) {
			if (ypos + (76 * x) >= 94 && ypos + (76 * x) + 30 < 400) {
				if (start == -1) {
					start = x;
				}

				finish = x;
			}
		}

		// Refresh list
		if (refresh_list != start) {
			if (icon1_img) GRRLIB_FreeTexture(icon1_img);
			if (icon2_img) GRRLIB_FreeTexture(icon2_img);
			if (icon3_img) GRRLIB_FreeTexture(icon3_img);
			if (icon4_img) GRRLIB_FreeTexture(icon4_img);
			if (icon5_img) GRRLIB_FreeTexture(icon5_img);

			// Load images as needed, store them into memory as well as text
			int c;
			for (c = start; c <= (start+4); c++) {

				icons_loaded++;

				if (homebrew_list[c].file_found == 0 || homebrew_list[c].file_found == 2) {

					if (homebrew_list[c].file_found == 0) {
						// Check image file size
						char img_path[100] = "sd:/apps/homebrew_browser/temp/";
						if (setting_use_sd == false) strcpy(img_path,"usb:/apps/homebrew_browser/temp/");
						strcat(img_path, homebrew_list[c].name);
						strcat(img_path, ".png");

						FILE *f = fopen(img_path, "rb");

						if (f == NULL) {
							homebrew_list[c].file_found = -1;
						}
						// Open file and check file length for changes
						else {
							fseek (f , 0, SEEK_END);
							int local_img_size = ftell (f);
							rewind(f);
							fclose(f);

							// Load image into memory if image size matches
							if (local_img_size == homebrew_list[c].img_size) {
								homebrew_list[c].file_found = load_file_to_memory(img_path, &homebrew_list[c].content);
							} else {
								homebrew_list[c].file_found = -1;
							}
						}
					}
				}
				if (homebrew_list[c].content != NULL) homebrew_list[c].file_found = 1;

				// Once we've gone through the first 8 images of the category (even if they weren't loaded), we wake the download icon thread and are done
				if (icons_loaded > 8) {
					changing_cat = false;
					icons_loaded = 0;
				}

				// Text list
				if (text_list[c].text == 0) {
					text_list[c].str_name = GRRLIB_TextToTexture(homebrew_list[c].app_name, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY);
					text_list[c].str_short_description = GRRLIB_TextToTexture(homebrew_list[c].app_short_description, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
					text_list[c].text = 1;

					if (c > string_count) string_count++;

					int a = c - 8;
					if (a > 0) {
						for (a = c - 7; a > 0; a--) {
							if (text_list[a].text != 0) {
								text_list[a].text = 0;

								GRRLIB_FreeTexture(text_list[a].str_name);
								GRRLIB_FreeTexture(text_list[a].str_short_description);
							}
						}
					}

					a = c + 8;
					if (a < array_length(homebrew_list)) {
						for (a = c + 7; a < array_length(homebrew_list); a++) {
							if (text_list[a].text != 0) {
								text_list[a].text = 0;

								GRRLIB_FreeTexture(text_list[a].str_name);
								GRRLIB_FreeTexture(text_list[a].str_short_description);
							}
						}
					}

				}
			}

			// Display images
			if (homebrew_list[start].file_found == 1 && strcmp(homebrew_list[start].name,"000") != 0) {
				icon1_img=GRRLIB_LoadTexture(homebrew_list[start].content);
			} else if (strlen(homebrew_list[start].name) >= 3 && strcmp(homebrew_list[start].name,"000") != 0) { icon1_img=GRRLIB_LoadTexture(no_image_png); }
			else { icon1_img=GRRLIB_LoadTexture(blank_png); }

			if (homebrew_list[(start+1)].file_found == 1 && strcmp(homebrew_list[(start+1)].name,"000") != 0) {
				icon2_img=GRRLIB_LoadTexture(homebrew_list[(start+1)].content);
			} else if (strlen(homebrew_list[(start+1)].name) >= 3 && strcmp(homebrew_list[(start+1)].name,"000") != 0) { icon2_img=GRRLIB_LoadTexture(no_image_png); }
			else { icon2_img=GRRLIB_LoadTexture(blank_png); }

			if (homebrew_list[(start+2)].file_found == 1 && strcmp(homebrew_list[(start+2)].name,"000") != 0) {
				icon3_img=GRRLIB_LoadTexture(homebrew_list[(start+2)].content);
			} else if (strlen(homebrew_list[(start+2)].name) >= 3 && strcmp(homebrew_list[(start+2)].name,"000") != 0) { icon3_img=GRRLIB_LoadTexture(no_image_png); }
			else { icon3_img=GRRLIB_LoadTexture(blank_png); }

			if (homebrew_list[(start+3)].file_found == 1 && strcmp(homebrew_list[(start+3)].name,"000") != 0) {
				icon4_img=GRRLIB_LoadTexture(homebrew_list[(start+3)].content);
			} else if (strlen(homebrew_list[(start+3)].name) >= 3 && strcmp(homebrew_list[(start+3)].name,"000") != 0) { icon4_img=GRRLIB_LoadTexture(no_image_png); }
			else { icon4_img=GRRLIB_LoadTexture(blank_png); }

			if (strlen(homebrew_list[(start+4)].name) >= 3 && homebrew_list[(start+4)].file_found == 1 && strcmp(homebrew_list[(start+4)].name,"000") != 0) {
				icon5_img=GRRLIB_LoadTexture(homebrew_list[(start+4)].content);
			} else if (strlen(homebrew_list[(start+4)].name) >= 3 && strcmp(homebrew_list[(start+4)].name,"000") != 0) { icon5_img=GRRLIB_LoadTexture(no_image_png); }
			else { icon5_img=GRRLIB_LoadTexture(blank_png); }

			// Name and Descriptions
			str_name = text_list[start].str_name;
			str_name1 = text_list[(start+1)].str_name;
			str_name2 = text_list[(start+2)].str_name;
			str_name3 = text_list[(start+3)].str_name;
			str_name4 = text_list[(start+4)].str_name;
			str_short_desc = text_list[start].str_short_description;
			str_short_desc1 = text_list[(start+1)].str_short_description;
			str_short_desc2 = text_list[(start+2)].str_short_description;
			str_short_desc3 = text_list[(start+3)].str_short_description;
			str_short_desc4 = text_list[(start+4)].str_short_description;

			refresh_list = start;
		}

		// Update ypos from ypos updated
		if (select_repo) ypos = ypos_updated;

		if (ACTIVITIES_current() == ACTIVITY_MAIN) {
			UI_roundedRect(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W, UI_PAGE_H, RES_COLOR_LIGHT_GRAY);
			UI_roundedRect(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W_SMALL, UI_PAGE_H, RES_COLOR_WHITE);

			// Dpad up / down
			if (!setting_wiiside) {
				if ((held & WPAD_BUTTON_DOWN || held_gc & PAD_BUTTON_DOWN) && ((strlen(homebrew_list[finish+1].name) > 1) || (select_repo && finish_updated+1 < repo_count))) ypos-= 4;
				if ((held & WPAD_BUTTON_UP || held_gc & PAD_BUTTON_UP) && ((!select_repo && ypos <= 140) || (select_repo && ypos <= 180))) ypos+= 4;
			}

			// Wiimote sideways
			if (held & WPAD_BUTTON_1) {
				if (held & WPAD_BUTTON_LEFT && ((strlen(homebrew_list[finish+1].name) > 1) || (select_repo && finish_updated+1 < repo_count))) ypos-= 5;
				if (held & WPAD_BUTTON_RIGHT && ((!select_repo && ypos <= 140) || (select_repo && ypos <= 180))) ypos+= 5;
			}

			// GC scrolling
			if ((strlen(homebrew_list[finish+1].name) > 1 && !select_repo) || (select_repo && finish_updated+1 < repo_count)) {
				if (PAD_SubStickY(0) < -18) ypos-= 2;
				if (PAD_SubStickY(0) < -28) ypos-= 1;
				if (PAD_SubStickY(0) < -38) ypos-= 1;
				if (PAD_SubStickY(0) < -48) ypos-= 1;
				if (PAD_SubStickY(0) < -60) ypos-= 5;
			}
			if ((select_repo == false && ypos <= 140) || (select_repo && ypos <= 180)) {
				if (PAD_SubStickY(0) > 18) ypos+= 2;
				if (PAD_SubStickY(0) > 28) ypos+= 1;
				if (PAD_SubStickY(0) > 38) ypos+= 1;
				if (PAD_SubStickY(0) > 48) ypos+= 1;
				if (PAD_SubStickY(0) > 60) ypos+= 5;
			}

			// Nunchuck scrolling
			struct expansion_t data;
			WPAD_Expansion(WPAD_CHAN_0, &data);
			if (data.type == WPAD_EXP_NUNCHUK) {
				if ((strlen(homebrew_list[finish+1].name) > 1 && !select_repo) || (select_repo && finish_updated+1 < repo_count)) {
					if (data.nunchuk.js.pos.y < 120) ypos-= 2;
					if (data.nunchuk.js.pos.y < 110) ypos-= 1;
					if (data.nunchuk.js.pos.y < 100) ypos-= 1;
					if (data.nunchuk.js.pos.y < 80) ypos-= 1;
					if (data.nunchuk.js.pos.y < 70) ypos-= 1;
					if (data.nunchuk.js.pos.y < 50) ypos-= 5;
				}
				if ((!select_repo && ypos <= 140) || (select_repo && ypos <= 180)) {
					if (data.nunchuk.js.pos.y > 140) ypos+= 2;
					if (data.nunchuk.js.pos.y > 150) ypos+= 1;
					if (data.nunchuk.js.pos.y > 160) ypos+= 1;
					if (data.nunchuk.js.pos.y > 180) ypos+= 1;
					if (data.nunchuk.js.pos.y > 190) ypos+= 1;
					if (data.nunchuk.js.pos.y > 210) ypos+= 5;
				}
			}

			// Update ypos updated from ypos
			if (select_repo) {
				ypos_updated = ypos;
				ypos = 142;
			}

			// Highlighting and display tick, question mark or plus
			GRRLIB_ClipDrawing(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W_SMALL, UI_PAGE_H);
			int b;
			for(b = 0; b <= 4; b++) {
				if (strlen(homebrew_list[(start + b)].name) >= 2 && strcmp(homebrew_list[(start + b)].name,"000") != 0) {
					if (UI_isOnSquare(ir, UI_PAGE_X, ypos + (76 * (start + b)), 530, 64)) {
						doRumble = true;
						UI_highlight(UI_PAGE_X, ypos + (76 * (start + b)), UI_PAGE_W_SMALL, 64);

						if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A)) {
							ACTIVITIES_open(ACTIVITY_APP);
							update_about = true;
							current_app = start + b;
							wait_a_press = 10;
						}
						if (pressed & WPAD_BUTTON_PLUS || pressed_gc & PAD_TRIGGER_R) {
							if (homebrew_list[(start + b)].local_app_size > 0) {
								homebrew_list[(start + b)].in_download_queue = 2;
							} else if (homebrew_list[(start + b)].local_app_size != homebrew_list[(start + b)].app_size) {
								homebrew_list[(start + b)].in_download_queue = true;
							}
						} else if (pressed & WPAD_BUTTON_MINUS || pressed_gc & PAD_TRIGGER_L) {
							homebrew_list[(start + b)].in_download_queue = false;
						}
					}

					if (homebrew_list[(start + b)].local_app_size > 0 && homebrew_list[(start + b)].in_download_queue == false) {
						if (homebrew_list[(start + b)].local_app_size == homebrew_list[(start + b)].app_size && homebrew_list[(start + b)].in_download_queue != 2) {
							GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
						}
						else if (homebrew_list[(start + b)].local_app_size != homebrew_list[(start + b)].app_size && homebrew_list[(start + b)].in_download_queue != 2 && homebrew_list[(start + b)].no_manage == false) {
							GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, app_question_img, 0, 1, 1, 0xFFFFFFFF);
						}

						if (homebrew_list[(start + b)].no_manage == true) {
							GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
						}
					}
					else if ((homebrew_list[(start + b)].app_time + 432000) > current_time && (strcmp (store_homebrew_list[0].name, homebrew_list[(start + b)].name) != 0)) {
						GRRLIB_DrawImg(468, ypos + (76 * (start + b)) - 6, app_new_img, 0, 1, 1, 0xFFFFFFFF);
					}

					if (homebrew_list[(start + b)].in_download_queue == true) {
						GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, stack_img, 0, 1, 1, RES_COLOR_GREEN);
					}
					else if (homebrew_list[(start + b)].in_download_queue == 2) {
						GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, stack_img, 0, 1, 1, RES_COLOR_RED);
					}

					if (strcmp (store_homebrew_list[0].name, homebrew_list[(start + b)].name) == 0 && (download_in_progress == true || extract_in_progress == true)) {
						GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, download_img, 0, 1, 1, RES_COLOR_BLUE);
					}
				}
			}

			// Display icons, text
			GRRLIB_DrawImg(60, ypos + (76 * start) + 4, icon1_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(210, ypos + (76 * start) + 4, str_name, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(210, ypos + (76 * start) + 30, str_short_desc, 0, 1.0, 1.0, 0xFFFFFFFF);

			GRRLIB_DrawImg(60, ypos + (76 * (start+1) + 4), icon2_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(210, ypos + (76 * (start+1)) + 4, str_name1, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(210, ypos + (76 * (start+1)) + 30, str_short_desc1, 0, 1.0, 1.0, 0xFFFFFFFF);

			GRRLIB_DrawImg(60, ypos + (76 * (start+2) + 4), icon3_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(210, ypos + (76 * (start+2)) + 4, str_name2, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(210, ypos + (76 * (start+2)) + 30, str_short_desc2, 0, 1.0, 1.0, 0xFFFFFFFF);

			GRRLIB_DrawImg(60, ypos + (76 * (start+3) + 4), icon4_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(210, ypos + (76 * (start+3)) + 4, str_name3, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(210, ypos + (76 * (start+3)) + 30, str_short_desc3, 0, 1.0, 1.0, 0xFFFFFFFF);

			GRRLIB_DrawImg(60, ypos + (76 * (start+4) + 4), icon5_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(210, ypos + (76 * (start+4)) + 4, str_name4, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(210, ypos + (76 * (start+4)) + 30, str_short_desc4, 0, 1.0, 1.0, 0xFFFFFFFF);

			GRRLIB_ClipReset();

			// Empty messages
			if (category_selection == 6 && strcmp(homebrew_list[0].name,"000") == 0 && wait_a_press == 0) {
				GRRLIB_DrawText(88, 159, STR_EMPTY_QUEUE_0, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 189, STR_EMPTY_QUEUE_1, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 219, STR_EMPTY_QUEUE_2, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 249, STR_EMPTY_QUEUE_3, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
			}
			if (category_selection == 5 && strcmp(homebrew_list[0].name,"000") == 0 && wait_a_press == 0) {
				GRRLIB_DrawText(88, 159, STR_EMPTY_SD_0, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 189, STR_EMPTY_SD_1, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 219, STR_EMPTY_SD_2, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
			}

			//Sorting icons
			if (UI_isOnImg(ir, UI_SORT_X, UI_SORT_1_Y, name_img)) {
				doRumble = true;
				GRRLIB_DrawImg(UI_SORT_X, UI_SORT_1_Y, name_img, 0, 1, 1, LIGHT_BTN_COLOR_HOVER);
				if (setting_tool_tip == true) { UI_drawTooltip(UI_SORT_X, UI_SORT_1_Y, STR_SORT_BY_NAME); }
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					// Free list strings
					free_string = true;
					if (sort_up_down == 6) { sort_up_down = 7; sort_by_name(1); refresh_list = -1; ypos = 142; }
					else { sort_up_down = 6; sort_by_name(0); refresh_list = -1; ypos = 142; }
				}
			} else {
				GRRLIB_DrawImg(UI_SORT_X, UI_SORT_1_Y, name_img, 0, 1, 1, LIGHT_BTN_COLOR);
			}
			if (UI_isOnImg(ir, UI_SORT_X, UI_SORT_2_Y, date_img)) {
				doRumble = true;
				GRRLIB_DrawImg(UI_SORT_X, UI_SORT_2_Y, date_img, 0, 1, 1, LIGHT_BTN_COLOR_HOVER);
				if (setting_tool_tip == true) { UI_drawTooltip(UI_SORT_X, UI_SORT_2_Y, STR_SORT_BY_DATE); }
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					// Free list strings
					free_string = true;
					if (sort_up_down == 0) { sort_up_down = 1; sort_by_date(1); refresh_list = -1; ypos = 142; }
					else { sort_up_down = 0; sort_by_date(0); refresh_list = -1; ypos = 142; }
				}
			} else {
				GRRLIB_DrawImg(UI_SORT_X, UI_SORT_2_Y, date_img, 0, 1, 1, LIGHT_BTN_COLOR);
			}

			if (sort_up_down == 0) GRRLIB_DrawImg(UI_SORT_ARROW_X, UI_SORT_2_Y + 20, sort_arrow_down_img, 0, 1, 1, LIGHT_BTN_COLOR);
			else if (sort_up_down == 1) GRRLIB_DrawImg(UI_SORT_ARROW_X, UI_SORT_2_Y + 4, sort_arrow_up_img, 0, 1, 1, LIGHT_BTN_COLOR);
			else if (sort_up_down == 6) GRRLIB_DrawImg(UI_SORT_ARROW_X, UI_SORT_1_Y + 20, sort_arrow_down_img, 0, 1, 1, LIGHT_BTN_COLOR);
			else if (sort_up_down == 7) GRRLIB_DrawImg(UI_SORT_ARROW_X, UI_SORT_1_Y + 4, sort_arrow_up_img, 0, 1, 1, LIGHT_BTN_COLOR);

			//Queue icon
			if (UI_isOnImg(ir, UI_SORT_X, UI_SORT_3_Y, stack_img)) {
				doRumble = true;
				if (category_selection == 6) {
					GRRLIB_DrawImg(UI_SORT_X, UI_SORT_3_Y, run_img, 0, 1, 1, LIGHT_BTN_COLOR_HOVER);
					if (setting_tool_tip) UI_drawTooltip(UI_SORT_X, UI_SORT_3_Y, STR_RUN);
				}
				else {
					GRRLIB_DrawImg(UI_SORT_X, UI_SORT_3_Y, stack_img, 0, 1, 1, LIGHT_BTN_COLOR_HOVER);
					if (setting_tool_tip) UI_drawTooltip(UI_SORT_X, UI_SORT_3_Y, STR_QUEUE);
				}
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && download_in_progress == false) {
					if (category_selection == 6) {
						if (setting_online == true && hide_apps_updated() == true && strcmp(homebrew_list[0].name,"000") != 0 && array_length (homebrew_list) >= 1) {
							clear_temp_list();
							updating = 0;
							free_update = true;
							download_queue_size();
						}
					} else {
						category_old_selection = category_selection;
						category_selection = 6;
						updated_cat = true;
					}
				}
			} else {
				if (category_selection == 6) GRRLIB_DrawImg(UI_SORT_X, UI_SORT_3_Y, run_img, 0, 1, 1, LIGHT_BTN_COLOR);
				else GRRLIB_DrawImg(UI_SORT_X, UI_SORT_3_Y, stack_img, 0, 1, 1, LIGHT_BTN_COLOR);
			}

			// Category Change DPAD
			if (!setting_wiiside) {
				if ((pressed & WPAD_BUTTON_LEFT || pressed_gc & PAD_BUTTON_LEFT) && category_selection > 0 && category_selection < 5 && updating == -1) {
					if ((category_selection-1 == 0 && array_length(demos_list) >= 1) || (category_selection-1 == 1 && array_length(emulators_list) >= 1) || (category_selection-1 == 2 && array_length(games_list) >= 1) || (category_selection-1 == 3 && array_length(media_list) >= 1) || (category_selection-1 == 4 && array_length(utilities_list) >= 1)) {
						category_old_selection = category_selection;
						category_selection--;
						updated_cat = true;
					}
				}
				if ((pressed & WPAD_BUTTON_RIGHT || pressed_gc & PAD_BUTTON_RIGHT) && category_selection < 4 && updating == -1) {
					if ((category_selection+1 == 0 && array_length(demos_list) >= 1) || (category_selection+1 == 1 && array_length(emulators_list) >= 1) || (category_selection+1 == 2 && array_length(games_list) >= 1) || (category_selection+1 == 3 && array_length(media_list) >= 1) || (category_selection+1 == 4 && array_length(utilities_list) >= 1)) {
						category_old_selection = category_selection;
						category_selection++;
						updated_cat = true;
					}
				}
			}
		}

		// Category selection
		UI_CAT_drawBar();
		if (ACTIVITIES_current() == ACTIVITY_MAIN) {
			UI_CAT_highlight(category_selection, BTN_COLOR_SELECTED);
		}
		if (UI_CAT_isOnCategory(ir, 0)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && updating == -1 && array_length(demos_list) >= 1) {
				category_old_selection = category_selection;
				category_selection = 0;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 1)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && updating == -1 && array_length(emulators_list) >= 1) {
				category_old_selection = category_selection;
				category_selection = 1;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 2)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && updating == -1 && array_length(games_list) >= 1) {
				category_old_selection = category_selection;
				category_selection = 2;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 3)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && updating == -1 && array_length(media_list) >= 1) {
				category_old_selection = category_selection;
				category_selection = 3;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 4)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && updating == -1 && array_length(utilities_list) >= 1) {
				category_old_selection = category_selection;
				category_selection = 4;
				updated_cat = true;
			}
		}

		// Changed category
		if ((category_selection != category_old_selection || ACTIVITIES_current() != ACTIVITY_MAIN) && updated_cat == true) {
			ypos = 142;
			icons_loaded = 0;
			WPAD_Rumble(WPAD_CHAN_0, 0);

			close_windows();
			ACTIVITIES_open(ACTIVITY_MAIN);

			// Tell downloading icon thread to sleep until we've loaded the new category
			if (download_icon > 0 && !download_in_progress) {
				changing_cat = true;
				while (!download_icon_sleeping) usleep(10000);
			}

			if (category_selection != 9) {
				update_lists();

				// Clear homebrew list
				clear_list();

				// Free list strings
				free_string = true;
			}

			// Update category
			if (category_selection == 0) {
				int i;
				for (i = 0; i < array_length (demos_list); i++) {
					homebrew_list[i] = demos_list[i];
				}
			} else if (category_selection == 1) {
				int i;
				for (i = 0; i < array_length (emulators_list); i++) {
					homebrew_list[i] = emulators_list[i];
				}
			} else if (category_selection == 2) {
				int i;
				for (i = 0; i < array_length (games_list); i++) {
					homebrew_list[i] = games_list[i];
				}
			} else if (category_selection == 3) {
				int i;
				for (i = 0; i < array_length (media_list); i++) {
					homebrew_list[i] = media_list[i];
				}
			} else if (category_selection == 4) {
				int i;
				for (i = 0; i < array_length (utilities_list); i++) {
					homebrew_list[i] = utilities_list[i];
				}
			} else if (category_selection == 5) {
				int i;
				for (i = 0; i < 4; i++) {
					strcpy(homebrew_list[i].name,"000");
				}
				int j = 0;
				for (i = 0; i < array_length (total_list); i++) {
					if (total_list[i].local_app_size > 0) {
						homebrew_list[j] = total_list[i];
						j++;
					}
				}
				wait_a_press = 7;
			} else if (category_selection == 6) {
				int i;
				for (i = 0; i < 4; i++) {
					strcpy(homebrew_list[i].name,"000");
				}
				int j = 0;
				for (i = 0; i < array_length (total_list); i++) {
					if (total_list[i].in_download_queue >= 1) {
						homebrew_list[j] = total_list[i];
						j++;
					}
				}
				wait_a_press = 7;
			}

			if (setting_hide_installed && category_selection != 5 && category_selection != 6) {
				hide_apps_installed();
			}

			if (sort_up_down == 0) sort_by_date(0);
			else if (sort_up_down == 1) sort_by_date(1);
			else if (sort_up_down == 6) sort_by_name(0);
			else if (sort_up_down == 7) sort_by_name(1);

			refresh_list = -1;
			updated_cat = false;
			changing_cat = false;
		}

		if (held & WPAD_BUTTON_1) {
			if (held & WPAD_BUTTON_DOWN) yy -= 3;
			else if (held & WPAD_BUTTON_UP) yy += 3;
			else if (held & WPAD_BUTTON_LEFT) xx -= 3;
			else if (held & WPAD_BUTTON_RIGHT) xx += 3;
		}

		if (pressed & WPAD_BUTTON_DOWN) yy -= 1;
		else if (pressed & WPAD_BUTTON_UP) yy += 1;
		else if (pressed & WPAD_BUTTON_LEFT) xx -= 1;
		else if (pressed & WPAD_BUTTON_RIGHT) xx += 1;


		// Main menu icons
		GRRLIB_DrawImg(UI_PAGE_X - 15, 16, logo_img, 0, 1, 1, 0xFFFFFFFF);
		if (UI_isOnImg(ir, 546, 16, list_img)) {
			doRumble = true;
			GRRLIB_DrawImg(546, 16, list_img, 0, 1, 1, BTN_COLOR_HOVER);
			if (setting_tool_tip == true) { UI_drawTooltip(546, 25, STR_INSTALLED_APPS); }
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && updating == -1) {
				category_old_selection = category_selection;
				category_selection = 5;
				updated_cat = true;
			}
		} else {
			GRRLIB_DrawImg(546, 16, list_img, 0, 1, 1, BTN_COLOR);
		}

		// Updating
		if (updating >= 0 && strcmp(homebrew_list[0].name,"000") != 0) {
			if (free_update) {
				char temp[50];
				sprintf (temp, "Processing %i/%i applications", new_updating + 1, array_length (homebrew_list));

				str_title_status = GRRLIB_TextToTexture(temp, FONTSIZE_SMALL, 0x575757);

				str_res_title = GRRLIB_TextToTexture(homebrew_list[new_updating].app_name, FONTSIZE_SMALL, 0x575757);

				free_update = false;
			}

			if ((updating < array_length (homebrew_list) && new_updating < array_length (homebrew_list)) || new_updating == 10000) {

				GRRLIB_DrawImg(220, 152, str_title_status, 0, 1.0, 1.0, 0xFFFFFFFF);
				GRRLIB_DrawImg(348 - (strlen(homebrew_list[updating].app_name) * 5.5), 196, str_res_title, 0, 1.0, 1.0, 0xFFFFFFFF);

				// Download
				if (download_in_progress) {
					int download_progress_count = (int) download_progress_counter / download_part_size;
					if (download_progress_count > 99) download_progress_count = 100;

					GRRLIB_Rectangle(139, 270, 400, 32, TOOLTIP_COLOR, true);
					GRRLIB_Rectangle(139, 270, download_progress_count * 4, 32, RES_COLOR_BLUE, true);
					GRRLIB_DrawText(208, 276, STR_DOWNLOADING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

					if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) {
						cancel_download = true;
						wait_a_press = 20;
					}

					// Download info
					char temp[50];
					double temp_download_progress_counter = download_progress_counter;
					double temp_total_app_size = homebrew_list[updating].total_app_size;
					sprintf (temp, "%3.2fMB / %3.2fMB", (temp_download_progress_counter/1000/1000), (temp_total_app_size/1000/1000));
					str_download_info = GRRLIB_TextToTexture(temp, FONTSIZE_SMALLER, 0x575757);
					GRRLIB_DrawImg(360, 278, str_download_info, 0, 1.0, 1.0, 0xFFFFFFFF);
					free_download_info = true;
				}

				// Extract
				if (extract_in_progress) {

					int extract_progress_count = (int) (zip_progress / extract_part_size);
					if (extract_progress_count > 100) extract_progress_count = 100;

					GRRLIB_Rectangle(139, 270, 400, 32, TOOLTIP_COLOR, true);
					GRRLIB_Rectangle(139, 270, extract_progress_count * 4, 32, RES_COLOR_BLUE, true);
					GRRLIB_DrawText(270, 276, STR_EXTRACTING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

					if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) {
						cancel_extract = true;
						wait_a_press = 20;
					}

					// Extract info
					char temp[50];
					int temp_extract_file_counter = unzip_file_counter;
					int temp_extract_file_count = unzip_file_count;
					sprintf (temp, "%i / %i", temp_extract_file_counter, temp_extract_file_count);
					str_download_info = GRRLIB_TextToTexture(temp, FONTSIZE_SMALLER, 0x575757);
					GRRLIB_DrawImg(410, 278, str_download_info, 0, 1.0, 1.0, 0xFFFFFFFF);
					free_download_info = true;
				}

				// Delete in progress
				if (delete_in_progress) {
					GRRLIB_DrawImg(290, 276, str_deleting, 0, 1.0, 1.0, 0xFFFFFFFF);
					if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) {
						cancel_delete = true;
						new_updating = 10000;
						wait_a_press = 20;
					}
				}

				// Cancel prompt
				if ((download_in_progress || extract_in_progress) && (cancel_download || cancel_extract) && !cancel_confirmed) {
					if (cancel_wait == 0) {
						cancel_wait = 10;
					} else if (cancel_wait == 1) {
						cancel_wait = -1;
					}

					if (cancel_wait != -1) {
						cancel_wait--;
					}

					GRRLIB_DrawImg(196, 250, cancel_download_prompt_img, 0, 1, 1, 0xFFFFFFFF);
					GRRLIB_DrawImg(213, 321, button_yes_img, 0, 1, 1, 0xFFFFFFFF);
					GRRLIB_DrawImg(353, 321, button_no_img, 0, 1, 1, 0xFFFFFFFF);

					if (ir.x > 199 && ir.x < 299 && ir.y > 320 && ir.y < 355) {
						doRumble = true;
						GRRLIB_DrawImg(213, 321, button_yes_highlight_img, 0, 1, 1, 0xFFFFFFFF);
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
							cancel_confirmed = true;
							new_updating = 10000;
							cancel_wait = 0;
						}
					}

					if (ir.x > 337 && ir.x < 440 && ir.y > 320 && ir.y < 355) {
						doRumble = true;
						GRRLIB_DrawImg(353, 321, button_no_highlight_img, 0, 1, 1, 0xFFFFFFFF);
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
							cancel_download = false;
							cancel_extract = false;
							cancel_wait = 0;
						}
					}

					if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) && cancel_wait == -1) {
						cancel_download = false;
						cancel_extract = false;
						cancel_wait = 0;
					}
				}

				// Overall progress
				int overall_progress = (int) (updating_current_size / updating_part_size);
				if (overall_progress > 98) overall_progress = 100;

				GRRLIB_Rectangle(139, 370, 400, 32, TOOLTIP_COLOR, true);
				GRRLIB_Rectangle(139, 370, overall_progress * 4, 32, RES_COLOR_BLUE, true);
				GRRLIB_DrawImg(270, 376, str_overall_progress, 0, 1.0, 1.0, 0xFFFFFFFF);


				if (new_updating != 10000 && (updating != new_updating || updating == 0) && !download_in_progress && !extract_in_progress && !delete_in_progress) {
					updating = new_updating;
					if (strlen(homebrew_list[updating].name) >= 3) {

						// Delete
						if (homebrew_list[updating].in_download_queue == 2) {
							delete_in_progress = true;
							selected_app = updating;
							initialise_delete();
						}

						// Downloading
						else {
							download_in_progress = true;
							selected_app = updating;
							add_to_stats();
							save_xml_name();
							initialise_download();
							if (homebrew_list[selected_app].local_app_size > 0 && homebrew_list[selected_app].local_app_size != homebrew_list[selected_app].app_size) {
								update_xml = 1;
							}
						}
					}
					free_update = true;
				}

				// Update XML
				if (update_xml == 2) {
					update_xml = 0;
					copy_xml_name();
				}
			}

			// Finished
			else if (!download_in_progress && !extract_in_progress && !delete_in_progress) {
				download_in_progress = false;
				extract_in_progress = false;
				delete_in_progress = false;
				cancel_extract = false;
				cancel_download = false;
				display_message_counter = 0;
				error_number = 0;
				updating = -1;
				new_updating = 0;
				cancel_confirmed = false;

				if (category_selection == 6) {
					clear_temp_list();

					int x;
					for (x = 0; x < array_length (homebrew_list); x++) {
						temp_list2[x] = homebrew_list[x];
					}

					clear_list();

					int i;
					for (i = 0; i < 4; i++) {
						strcpy(homebrew_list[i].name,"000");
					}

					i = 0;
					int j = 0;
					for (i = 0; i < array_length (temp_list2); i++) {
						homebrew_list[j] = temp_list2[i];
						j++;
					}

					category_old_selection = 10;
					category_selection = 9;
					updated_cat = true;
					refresh_list = -1;
					free_string = true;
				}
			}

			// Download failed or extracting failed?
			if (download_in_progress == -1 || extract_in_progress == -1 || delete_in_progress == -1) {

				// Display what error occured
				if (error_number == 1) {
					GRRLIB_DrawImg(210, 276, str_download_zip_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				}
				if (error_number == 2) {
					GRRLIB_DrawImg(210, 276, str_create_folder_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				}
				if (error_number == 3) {
					GRRLIB_DrawImg(210, 276, str_download_icon_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				}
				if (error_number == 4) {
					GRRLIB_DrawImg(210, 276, str_extract_zip_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				}
				if (error_number == 5) {
					GRRLIB_DrawImg(190, 276, str_boot_file_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				}
				if (error_number == 6) {
					GRRLIB_DrawImg(210, 396, str_delete_file_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				}
				if (error_number == 9) {
					GRRLIB_DrawImg(240, 276, str_free_space_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				}
				if (error_number == 10) {
					GRRLIB_DrawImg(240, 276, str_no_wifi, 0, 1.0, 1.0, 0xFFFFFFFF);
				}

				// Reset vars
				if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) && wait_a_press == 0) {
					download_in_progress = false;
					extract_in_progress = false;
					delete_in_progress = false;
					cancel_extract = false;
					cancel_download = false;
					display_message_counter = 0;
					error_number = 0;
					cancel_confirmed = false;
					updating = 9999;
				}
			}

			if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) && !download_in_progress && !extract_in_progress && display_message_counter == 0 && wait_a_press == 0) {
				updating = -1;
				new_updating = 0;
				updated_cat = true;
				refresh_list = -1;
				cancel_confirmed = false;
			}
		}


		// About
		if (ACTIVITIES_current() == ACTIVITY_APP) {
			if (update_about) {
				str_res_title = GRRLIB_TextToTexture(homebrew_list[current_app].app_name, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY);
				str_res_author = GRRLIB_TextToTexture(homebrew_list[current_app].app_author, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
				str_res_version = GRRLIB_TextToTexture(homebrew_list[current_app].app_version, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);

				char temp[50];
				if (homebrew_list[current_app].app_total_size > 0 && homebrew_list[current_app].app_total_size < 1048576) {
					int appsize = homebrew_list[current_app].app_total_size / 1024;
					sprintf (temp, "%i KB", appsize);
				}
				else {
					float appsize = (float) (homebrew_list[current_app].app_total_size / 1024) / 1024;
					sprintf (temp, "%1.1f MB", appsize);
				}
				str_res_size = GRRLIB_TextToTexture(temp, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);


				app_time = homebrew_list[current_app].app_time;
				char timebuf[50];
				timeinfo = localtime ( &app_time );
				strftime (timebuf,50,"%d %b %Y",timeinfo);
				str_res_date = GRRLIB_TextToTexture(timebuf, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);

				const int text_size = sizeof(homebrew_list[0].app_description);
				char text_description[text_size];
				strcpy(text_description, homebrew_list[current_app].app_description);

				int s;
				int l = 0;
				for(s = strlen(text_description); s < text_size; s++) {
					text_description[s] = text_white[0];
					l = text_size > s ? s : text_size;
				}
				text_description[l] = '\0';

				int x;
				int count = 0;
				int textrow = 0;
				int offset = 0;
				char test[80];
				for (x = 0; x < strlen(text_description); x++) {
					test[count] = text_description[x];
					count++;
					if (x >= (55 * (textrow+1)) && x <= (75 * (textrow+1)) && text_description[x] == ' ' && textrow == 0) {
						test[count] = '\0';
						string1 = GRRLIB_TextToTexture(test, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
						textrow = 1;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 1) {
						test[count] = '\0';
						string2 = GRRLIB_TextToTexture(test, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
						textrow = 2;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 2) {
						test[count] = '\0';
						string3 = GRRLIB_TextToTexture(test, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
						textrow = 3;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 3) {
						test[count] = '\0';
						string4 = GRRLIB_TextToTexture(test, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
						textrow = 4;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 4) {
						test[count] = '\0';
						string5 = GRRLIB_TextToTexture(test, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
						count = 0;
						textrow = 5;
						break;
					}
				}

				update_about = false;
			}

			GRRLIB_DrawImg(330 - (strlen(homebrew_list[current_app].app_name) * 5.5), 140, str_res_title, 0, 1.0, 1.0, 0xFFFFFFFF);

			GRRLIB_DrawImg(70, 170, string1, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(70, 190, string2, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(70, 210, string3, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(70, 230, string4, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(70, 250, string5, 0, 1.0, 1.0, 0xFFFFFFFF);

			GRRLIB_DrawText(70, 290, STR_AUTHOR, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(70, 310, STR_VERSION, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(70, 330, STR_SIZE, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(70, 350, STR_DATE, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);

			GRRLIB_DrawImg(140, 290, str_res_author, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(140, 310, str_res_version, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(140, 330, str_res_date, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(140, 350, str_res_size, 0, 1.0, 1.0, 0xFFFFFFFF);

			if (strstr(homebrew_list[current_app].app_controllers, "wwww")) {
				GRRLIB_DrawImg(290, 330, control_wiimote_4_img, 0, 1, 1, 0xFFFFFFFF);
			}
			else if (strstr(homebrew_list[current_app].app_controllers, "www")) {
				GRRLIB_DrawImg(290, 330, control_wiimote_3_img, 0, 1, 1, 0xFFFFFFFF);
			}
			else if (strstr(homebrew_list[current_app].app_controllers, "ww")) {
				GRRLIB_DrawImg(290, 330, control_wiimote_2_img, 0, 1, 1, 0xFFFFFFFF);
			}
			else if (strstr(homebrew_list[current_app].app_controllers, "w")) {
				GRRLIB_DrawImg(290, 330, control_wiimote_img, 0, 1, 1, 0xFFFFFFFF);
			}
			else { GRRLIB_DrawImg(290, 330, control_wiimote_img, 0, 1, 1, 0xFFFFFF3C); }
			if (strstr(homebrew_list[current_app].app_controllers, "n")) {
				GRRLIB_DrawImg(310, 330, control_nunchuck_img, 0, 1, 1, 0xFFFFFFFF);
			} else { GRRLIB_DrawImg(310, 330, control_nunchuck_img, 0, 1, 1, 0xFFFFFF3C); }
			if (strstr(homebrew_list[current_app].app_controllers, "c")) {
				GRRLIB_DrawImg(346, 330, control_classic_img, 0, 1, 1, 0xFFFFFFFF);
			} else { GRRLIB_DrawImg(346, 330, control_classic_img, 0, 1, 1, 0xFFFFFF3C); }
			if (strstr(homebrew_list[current_app].app_controllers, "g")) {
				GRRLIB_DrawImg(395, 330, control_gcn_img, 0, 1, 1, 0xFFFFFFFF);
			} else { GRRLIB_DrawImg(395, 330, control_gcn_img, 0, 1, 1, 0xFFFFFF3C); }
			if (strstr(homebrew_list[current_app].app_controllers, "z")) {
				GRRLIB_DrawImg(455, 330, control_zapper_img, 0, 1, 1, 0xFFFFFFFF);
			} else { GRRLIB_DrawImg(455, 330, control_zapper_img, 0, 1, 1, 0xFFFFFF3C); }
			if (strstr(homebrew_list[current_app].app_controllers, "k")) {
				GRRLIB_DrawImg(544, 330, control_keyboard_img, 0, 1, 1, 0xFFFFFFFF);
			} else { GRRLIB_DrawImg(544, 330, control_keyboard_img, 0, 1, 1, 0xFFFFFF3C); }
			GRRLIB_DrawText(489, 290, STR_SDHC, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			if (strstr(homebrew_list[current_app].app_controllers, "s")) {
				GRRLIB_DrawText(553, 290, STR_YES, FONTSIZE_SMALLER, RES_COLOR_GREEN);
			} else { GRRLIB_DrawText(553, 290, STR_NO, FONTSIZE_SMALLER, RES_COLOR_RED); }

			if ((!download_in_progress && !extract_in_progress && !delete_in_progress) || (strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) != 0)) {
				// Download or updated enabled?
				if (homebrew_list[current_app].local_app_size > 0) {
					if (homebrew_list[current_app].local_app_size != homebrew_list[current_app].app_size && download_arrow == 0 && !homebrew_list[current_app].no_manage) {
						if (UI_button(ir, 340, 385, STR_UPDATE)) {
							doRumble = true;
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0 && download_in_progress == false && extract_in_progress == false && delete_in_progress == false) {
								download_in_progress = true;
								selected_app = current_app;
								add_to_stats();
								save_xml_name();
								initialise_download();
								if (homebrew_list[selected_app].local_app_size > 0 && homebrew_list[selected_app].local_app_size != homebrew_list[selected_app].app_size) {
									update_xml = 1;
								}
							}
						}
					}
					else if (homebrew_list[current_app].local_app_size > 0 && download_arrow == 0) {
						UI_drawButton(340, 385, STR_UPDATE, 2);
					}

					if (download_arrow == 0) {
						if (UI_button(ir, 485, 385, STR_DELETE)) {
							doRumble = true;
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0 && download_in_progress == false && extract_in_progress == false && delete_in_progress == false) {
								delete_in_progress = true;
								selected_app = current_app;
								initialise_delete();
							}
						}
					}

					if (download_arrow != 0) {
						if (homebrew_list[current_app].no_manage == false) {
							GRRLIB_DrawImg(490, 385, button_yes_img, 0, 1, 1, 0xFFFFFF96);

							if (ir.x > 480 && ir.x < 580 && ir.y > 385 && ir.y < 420) {
								doRumble = true;
								GRRLIB_DrawImg(490, 385, button_yes_img, 0, 1, 1, 0xFFFFFFFF);
								if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0 && download_in_progress == false && extract_in_progress == false && delete_in_progress == false) {
									homebrew_list[current_app].no_manage = true;
								}
							}
						}
						else {
							GRRLIB_DrawImg(490, 385, button_no_img, 0, 1, 1, 0xFFFFFF96);

							if (ir.x > 480 && ir.x < 580 && ir.y > 385 && ir.y < 420) {
								doRumble = true;
								GRRLIB_DrawImg(490, 385, button_no_img, 0, 1, 1, 0xFFFFFFFF);
								if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0 && download_in_progress == false && extract_in_progress == false && delete_in_progress == false) {
									homebrew_list[current_app].no_manage = false;
								}
							}
						}
					}
				}
				else {
					if (download_in_progress && strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) != 0) {
						UI_drawButton(340, 385, STR_DOWNLOAD, 2);
					} else {
						if (UI_button(ir, 340, 385, STR_DOWNLOAD)) {
							doRumble = true;
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0 && download_in_progress == false && extract_in_progress == false && delete_in_progress == false) {
								download_in_progress = true;
								selected_app = current_app;
								add_to_stats();
								initialise_download();
							}
						}
					}

					if (download_arrow == 0) {
						UI_drawButton(485, 385, STR_DELETE, 2);
					}
				}
			}

			// Downloading in progress, display progress
			if (download_in_progress && strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) == 0) {

				int download_progress_count = (int) download_progress_counter / download_part_size;
				if (download_progress_count > 99) download_progress_count = 100;

				GRRLIB_Rectangle(139, 390, 400, 32, TOOLTIP_COLOR, true);
				GRRLIB_Rectangle(139, 390, download_progress_count * 4, 32, RES_COLOR_BLUE, true);
				GRRLIB_DrawText(208, 396, STR_DOWNLOADING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

				if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) {
					cancel_download = true;
				}

				// Download info
				char temp[50];
				double temp_download_progress_counter = download_progress_counter;
				double temp_total_app_size = store_homebrew_list[0].total_app_size;
				sprintf (temp, "%3.2fMB / %3.2fMB", (temp_download_progress_counter/1000/1000), (temp_total_app_size/1000/1000));
				str_download_info = GRRLIB_TextToTexture(temp, FONTSIZE_SMALLER, 0x575757);
				GRRLIB_DrawImg(360, 398, str_download_info, 0, 1.0, 1.0, 0xFFFFFFFF);
				free_download_info = true;
			}

			// Extracting in progress, display progress
			if (extract_in_progress && strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) == 0) {

				int extract_progress_count = (int) (zip_progress / extract_part_size);
				if (extract_progress_count > 100) extract_progress_count = 100;

				GRRLIB_Rectangle(139, 390, 400, 32, TOOLTIP_COLOR, true);
				GRRLIB_Rectangle(139, 390, extract_progress_count * 4, 32, RES_COLOR_BLUE, true);
				GRRLIB_DrawText(270, 396, STR_EXTRACTING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

				if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) {
					cancel_extract = true;
				}

				// Extract info
				char temp[50];
				int temp_extract_file_counter = unzip_file_counter;
				int temp_extract_file_count = unzip_file_count;
				sprintf (temp, "%i / %i", temp_extract_file_counter, temp_extract_file_count);
				str_download_info = GRRLIB_TextToTexture(temp, FONTSIZE_SMALLER, 0x575757);
				GRRLIB_DrawImg(410, 398, str_download_info, 0, 1.0, 1.0, 0xFFFFFFFF);
				free_download_info = true;
			}

			// Cancel prompt
			if ((download_in_progress || extract_in_progress) && (cancel_download || cancel_extract) && !cancel_confirmed) {
				if (cancel_wait == 0) cancel_wait = 10;
				else if (cancel_wait == 1) cancel_wait = -1;

				if (cancel_wait != -1) cancel_wait--;

				GRRLIB_DrawImg(196, 250, cancel_download_prompt_img, 0, 1, 1, 0xFFFFFFFF);
				GRRLIB_DrawImg(213, 321, button_yes_img, 0, 1, 1, 0xFFFFFFFF);
				GRRLIB_DrawImg(353, 321, button_no_img, 0, 1, 1, 0xFFFFFFFF);

				if (ir.x > 199 && ir.x < 299 && ir.y > 320 && ir.y < 355) {
					doRumble = true;
					GRRLIB_DrawImg(213, 321, button_yes_highlight_img, 0, 1, 1, 0xFFFFFFFF);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						cancel_confirmed = true;
						cancel_wait = 0;
					}
				}

				if (ir.x > 337 && ir.x < 440 && ir.y > 320 && ir.y < 355) {
					doRumble = true;
					GRRLIB_DrawImg(353, 321, button_no_highlight_img, 0, 1, 1, 0xFFFFFFFF);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						cancel_download = false;
						cancel_extract = false;
						cancel_wait = 0;
					}
				}

				if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) && cancel_wait == -1) {
					cancel_download = false;
					cancel_extract = false;
					cancel_wait = 0;
				}
			}

			if (!download_in_progress && !extract_in_progress) {
				cancel_download = false;
				cancel_extract = false;
				cancel_wait = 0;
			}

			// Delete in progress
			if (delete_in_progress && strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) == 0) {
				GRRLIB_DrawImg(270, 396, str_deleting, 0, 1.0, 1.0, 0xFFFFFFFF);
			}


			// Download failed or extracting failed or deleting failed?
			if ((download_in_progress == -1 || extract_in_progress == -1 || delete_in_progress == -1) && display_message_counter == 0) {

				// Display what error occured
				if (error_number == 2 || error_number == 3 || error_number == 4 || error_number == 5) {
					initialise_delete();
				}

				cancel_extract = false;
				display_message_counter = 200;
			}

			// Display error message for some time
			if (display_message_counter > 0 && strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) == 0) {
				display_message_counter--;

				// Display what error occured
				if (error_number == 1) GRRLIB_DrawImg(190, 396, str_download_zip_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 2) GRRLIB_DrawImg(190, 396, str_create_folder_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 3) GRRLIB_DrawImg(190, 396, str_download_icon_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 4) GRRLIB_DrawImg(190, 396, str_extract_zip_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 5) GRRLIB_DrawImg(190, 396, str_boot_file_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 6) GRRLIB_DrawImg(190, 396, str_delete_file_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 7) GRRLIB_DrawImg(210, 396, str_delete_folder_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 8) GRRLIB_DrawImg(190, 396, str_delete_app_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 9) GRRLIB_DrawImg(190, 396, str_free_space_failed, 0, 1.0, 1.0, 0xFFFFFFFF);
				else if (error_number == 10) GRRLIB_DrawImg(190, 396, str_no_wifi, 0, 1.0, 1.0, 0xFFFFFFFF);

				if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1 || pressed_gc & PAD_BUTTON_B) {
					display_message_counter = 1;
				}

				// Reset vars
				if (display_message_counter == 1) {
					download_in_progress = false;
					extract_in_progress = false;
					delete_in_progress = false;
					cancel_download = false;
					cancel_extract = false;
					display_message_counter = 0;
					error_number = 0;
					cancel_confirmed = false;
					download_arrow = 0;
				}
			}
		}

		// Menu
		if (ACTIVITIES_current() == ACTIVITY_MENU) {
			GRRLIB_DrawImg(82, 146, home_bg_img, 0, 1, 1, 0xFFFFFFFF);

			if (UI_blockButton(ir, 283, UI_MENU_BTN_1_Y, STR_ABOUT)) {
				doRumble = true;
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					ACTIVITIES_open(ACTIVITY_ABOUT);
				}
			}

			if (UI_blockButton(ir, 283, UI_MENU_BTN_2_Y, STR_CONTROLLER)) {
				doRumble = true;
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					ACTIVITIES_open(ACTIVITY_HELP_CONTROLLER);
				}
			}

			if (UI_blockButton(ir, 283, UI_MENU_BTN_3_Y, STR_SETTINGS)) {
				doRumble = true;
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					ACTIVITIES_open(ACTIVITY_SETTINGS);
				}
			}

			if (UI_blockButton(ir, 283, UI_MENU_BTN_4_Y, STR_RETURN_TO_WII_MENU)) {
				doRumble = true;
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					WPAD_Rumble(WPAD_CHAN_0, 0);
					WPAD_Rumble(WPAD_CHAN_0, 0);
					exiting = true;
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
				doRumble = true;
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					WPAD_Rumble(WPAD_CHAN_0, 0);
					WPAD_Rumble(WPAD_CHAN_0, 0);
					exiting = true;
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

		// About
		if (ACTIVITIES_current() == ACTIVITY_ABOUT) {
			GRRLIB_DrawImg(UI_PAGE_X, 128, help_about_img, 0, 1, 1, 0xFFFFFFFF);
		}

		// Help Controller
		if (ACTIVITIES_current() == ACTIVITY_HELP_CONTROLLER) {
			GRRLIB_DrawImg(UI_PAGE_X, 128, help_controller_img, 0, 1, 1, 0xFFFFFFFF);
		}

		// Settings
		if (ACTIVITIES_current() == ACTIVITY_SETTINGS) {
			GRRLIB_DrawImg(82, 146, gear_bg_img, 0, 1, 1, 0xFFFFFFFF);

			if (menu_section == 0) {
				if (UI_isOnSquare(ir, 104, 146, 440, 44)) {
					doRumble = true;
					UI_highlight(104, 146, 440, 44);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						if (setting_sd_card == true) setting_sd_card = false;
						else setting_sd_card = true;
					}
				} else if (UI_isOnSquare(ir, 104, 196, 440, 44)) {
					doRumble = true;
					UI_highlight(104, 196, 440, 44);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						if (setting_hide_installed == true) setting_hide_installed = false;
						else setting_hide_installed = true;
					}
				} else if (UI_isOnSquare(ir, 104, 246, 440, 44)) {
					doRumble = true;
					UI_highlight(104, 246, 440, 44);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						if (setting_rumble == true) setting_rumble = false;
						else setting_rumble = true;
					}
				} else if (UI_isOnSquare(ir, 104, 296, 440, 44)) {
					doRumble = true;
					UI_highlight(104, 296, 440, 44);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						if (setting_update_icon == true) setting_update_icon = false;
						else setting_update_icon = true;
					}
				} else if (UI_isOnSquare(ir, 104, 346, 440, 44)) {
					doRumble = true;
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
						doRumble = true;
						UI_highlight(104, 146, 440, 44);
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
							if (setting_tool_tip == true) setting_tool_tip = false;
							else setting_tool_tip = true;
						}
					} else if (UI_isOnSquare(ir, 104, 196, 440, 44)) {
						doRumble = true;
						UI_highlight(104, 196, 440, 44);
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
							if (setting_use_sd == true) setting_use_sd = false;
							else setting_use_sd = true;
						}
					} else if (UI_isOnSquare(ir, 104, 246, 440, 44)) {
						doRumble = true;
						UI_highlight(104, 246, 440, 44);
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
							select_repo = true;
							wait_a_press = 10;
						}
					} else if (UI_isOnSquare(ir, 104, 296, 440, 44)) {
						doRumble = true;
						UI_highlight(104, 296, 440, 44);
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
							select_category = true;
							wait_a_press = 10;
						}
					} else if (UI_isOnSquare(ir, 104, 346, 440, 44)) {
						doRumble = true;
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
					doRumble = true;
					UI_highlight(104, 146, 440, 44);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						if (setting_disusb == true) setting_disusb = false;
						else setting_disusb = true;
					}
				} else if (UI_isOnSquare(ir, 104, 196, 440, 44)) {
					doRumble = true;
					UI_highlight(104, 196, 440, 44);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
						if (setting_wiiside == true) setting_wiiside = false;
						else setting_wiiside = true;
					}
				} else if (UI_isOnSquare(ir, 104, 246, 440, 44)) {
					doRumble = true;
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
					doRumble = true;
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
					doRumble = true;
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

		// Home button press
		if ((pressed & WPAD_BUTTON_HOME || pressed_gc & PAD_BUTTON_START) && updating == -1 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
			if (ACTIVITIES_current() == ACTIVITY_MENU) ACTIVITIES_goBack();
			else {
				close_windows();
				ACTIVITIES_open(ACTIVITY_MENU);
			}
			wait_a_press = 10;
		}

		// B button press
		if ((pressed & WPAD_BUTTON_B || pressed_gc & PAD_BUTTON_B) && updating == -1 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
			if (!close_windows()) ACTIVITIES_goBack();
			wait_a_press = 10;
		}

		// Button
		if (wait_a_press > 0) wait_a_press--;

		// Update XML
		if (update_xml == 2) {
			update_xml = 0;
			copy_xml_name();
		}

		// Select Repo
		if (select_repo) {
			GRRLIB_DrawImg(123, 148, apps_repo_img, 0, 1, 1, 0xFFFFFFFF);

			if (!repo_texted) {
				int u;
				for (u = 0; u < repo_count; u++) {
					repo_list[u].str_text = GRRLIB_TextToTexture(repo_list[u].name, FONTSIZE_SMALLER, 0x575757);
				}
				repo_texted = true;
			}

			int x;
			start_updated = -1;
			for (x = 0; x < repo_count; x++) {
				if (ypos_updated + (25 * x) >= 175 && ypos_updated + (25 * x) + 30 < 425) {
					if (start_updated == -1) {
						start_updated = x;
					}
					if (start_updated >= 0) {

						if (UI_isOnSquare(ir, 150, ypos_updated + (25 * x) - 1, 328, 24)) {
							doRumble = true;
							UI_highlight(150, ypos_updated + (25 * x) - 1, 328, 24);
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
								setting_repo = x;
							}
						}

						GRRLIB_DrawImg(150, (x * 25) + ypos_updated, repo_list[x].str_text, 0, 1.0, 1.0, 0xFFFFFFFF);

						if (setting_repo == x) {
							GRRLIB_DrawImg(150 + strlen(repo_list[x].name) * 10, ypos_updated + (25 * x), app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);
						}

					}
					finish_updated = x;
				}
			}

			GRRLIB_DrawImg(426, 156, updated_close_img, 0, 1, 1, 0xFFFFFFFF);

			if (ir.x > 412 && ir.x < 460 && ir.y > 155 && ir.y < 173) {
				doRumble = true;
				GRRLIB_DrawImg(426, 156, updated_close_highlight_img, 0, 1, 1, 0xFFFFFFFF);
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					select_repo = false;
					wait_a_press = 10;
				}
			}
		}

		// Select category
		if (select_category) {
			GRRLIB_DrawImg(123, 148, apps_start_cat_img, 0, 1, 1, 0xFFFFFFFF);

			GRRLIB_DrawImg(180, 195, str_cat_1, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(180, 225, str_cat_2, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(180, 255, str_cat_3, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(180, 285, str_cat_4, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(180, 315, str_cat_5, 0, 1.0, 1.0, 0xFFFFFFFF);

			if (UI_isOnSquare(ir, 165, 197, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 197, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					setting_category = 0;
				}
			}

			if (UI_isOnSquare(ir, 165, 227, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 227, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					setting_category = 1;
				}
			}

			if (UI_isOnSquare(ir, 165, 257, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 257, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					setting_category = 2;
				}
			}

			if (UI_isOnSquare(ir, 165, 287, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 287, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					setting_category = 3;
				}
			}

			if (UI_isOnSquare(ir, 165, 317, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 317, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					setting_category = 4;
				}
			}

			if (setting_category == 0) GRRLIB_DrawImg(380, 198, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);
			else if (setting_category == 1) GRRLIB_DrawImg(380, 228, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);
			else if (setting_category == 2) GRRLIB_DrawImg(380, 258, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);
			else if (setting_category == 3) GRRLIB_DrawImg(380, 288, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);
			else if (setting_category == 4) GRRLIB_DrawImg(380, 318, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);

			GRRLIB_DrawImg(386, 156, updated_close_img, 0, 1, 1, 0xFFFFFFFF);

			if (ir.x > 372 && ir.x < 420 && ir.y > 155 && ir.y < 173) {
				doRumble = true;
				GRRLIB_DrawImg(386, 156, updated_close_highlight_img, 0, 1, 1, 0xFFFFFFFF);
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) {
					select_category = false;
					wait_a_press = 10;
				}
			}
		}

		// Select sort
		if (select_sort) {
			GRRLIB_DrawImg(123, 148, apps_start_sort_img, 0, 1, 1, 0xFFFFFFFF);

			GRRLIB_DrawImg(180, 195, str_sort_1, 0, 1.0, 1.0, 0xFFFFFFFF);
			GRRLIB_DrawImg(180, 225, str_sort_2, 0, 1.0, 1.0, 0xFFFFFFFF);

			if (UI_isOnSquare(ir, 165, 197, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 197, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					setting_sort = 0;
				}
			}

			if (UI_isOnSquare(ir, 165, 227, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 227, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					setting_sort = 1;
				}
			}

			if (setting_sort == 0) GRRLIB_DrawImg(380, 198, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);
			else if (setting_sort == 1) GRRLIB_DrawImg(380, 228, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);

			GRRLIB_DrawImg(386, 156, updated_close_img, 0, 1, 1, 0xFFFFFFFF);

			if (ir.x > 372 && ir.x < 420 && ir.y > 155 && ir.y < 173) {
				doRumble = true;
				GRRLIB_DrawImg(386, 156, updated_close_highlight_img, 0, 1, 1, 0xFFFFFFFF);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2 || pressed_gc & PAD_BUTTON_A) && wait_a_press == 0) {
					select_sort = false;
					wait_a_press = 10;
				}
			}
		}

		// SD card usage
		if (setting_sd_card && sd_card_update) {
			WPAD_Rumble(WPAD_CHAN_0, 0);
			struct statvfs fiData;
			if (setting_use_sd) {
				if (statvfs("sd:/",&fiData) >= 0) {
					sd_card_free = fiData.f_bfree;
					sd_card_free = round(sd_card_free * fiData.f_bsize);
					sd_card_free = sd_card_free / 1000 / 1000;
				}
			} else if (!setting_use_sd) {
				if (statvfs("usb:/",&fiData) >= 0) {
					sd_card_free = fiData.f_bfree;
					sd_card_free = round(sd_card_free * fiData.f_bsize);
					sd_card_free = sd_card_free / 1000 / 1000;
				}
			}

			if (free_sd_size) {
				GRRLIB_FreeTexture(str_sd_card);
				free_sd_size = false;
			}

			char temp[50];
			sprintf (temp, "%lli MB Free", sd_card_free);

			str_sd_card = GRRLIB_TextToTexture(temp, FONTSIZE_SMALLER, 0x9d9d9d);
			sd_card_update = false;
			free_sd_size = true;
		}

		if (setting_sd_card) {
			GRRLIB_DrawImg(468, 441, str_sd_card, 0, 1.0, 1.0, 0xFFFFFFFF);
		}

		// Rumble
		if (setting_rumble) {
			if (doRumble) {
				if (rumble_count < 3) {
					if (ir.valid) WPAD_Rumble(WPAD_CHAN_0, 1);
					rumble_count++;
				} else {
					WPAD_Rumble(WPAD_CHAN_0, 0);
					doRumble = false;
				}
			} else {
				rumble_count = 0;
			}
		}

		// Icon load info
		if (download_icon > 0 && !download_in_progress && !extract_in_progress) {
			char temp[50];
			sprintf (temp, "Checking icon %i / %i", download_icon, array_length(total_list));
			str_icon_info = GRRLIB_TextToTexture(temp, FONTSIZE_SMALLER, 0x9d9d9d);
			GRRLIB_DrawImg(248, 441, str_icon_info, 0, 1.0, 1.0, 0xFFFFFFFF);
			free_icon_info = true;
		}

		if (download_in_progress && updating == -1 && strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) != 0) {
			GRRLIB_DrawText(248, 441, STR_DOWNLOADING_SMALL, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY_DARK);
		}
		if (extract_in_progress && updating == -1 && strcmp (store_homebrew_list[0].name, homebrew_list[current_app].name) != 0) {
			GRRLIB_DrawText(248, 441, STR_EXTRACTING_SMALL, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY_DARK);
		}

		// Draw the IR pointer
		GRRLIB_DrawImg(ir.x - (*mouse_img).w / 2, ir.y - (*mouse_img).h / 2, mouse_img, ir.angle, 1, 1, 0xFFFFFFFF);
		if (ir.valid || (ir.valid && held & WPAD_BUTTON_B) || held_gc & PAD_BUTTON_B) {
			gc_control_used = false;
		}

		GRRLIB_Render();

		// Free the strings from the list
		if (free_string) {
			int a = 0;
			for (a = 0; a < string_count; a++) {
				if (text_list[a].text != 0) {
					text_list[a].text = 0;

					GRRLIB_FreeTexture(text_list[a].str_name);
					GRRLIB_FreeTexture(text_list[a].str_short_description);
				}

			}
			free_string = false;
			string_count = 6;
		}

		// Icon loading info
		if (free_icon_info && download_icon > 0) {
			GRRLIB_FreeTexture(str_icon_info);
			free_icon_info = false;
		}

		// Download size progress
		if (free_download_info) {
			GRRLIB_FreeTexture(str_download_info);
			free_download_info = false;
		}

		// Exit
		if ((held & WPAD_BUTTON_HOME || held_gc & PAD_BUTTON_START) && held_counter >= 1) {
			held_counter++;
		}
		if (pressed & WPAD_BUTTON_HOME || pressed_gc & PAD_BUTTON_START) {
			held_counter = 1;
		}
		if (held_counter > 20) {
			WPAD_Rumble(WPAD_CHAN_0, 0);
			WPAD_Rumble(WPAD_CHAN_0, 0);
			update_lists();
			exiting = true;
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

		if (HWButton != -1) {
			WPAD_Rumble(WPAD_CHAN_0, 0);
			WPAD_Rumble(WPAD_CHAN_0, 0);
			update_lists();
			exiting = true;
			usleep(300000);
			if (download_icon > 0) {
				changing_cat = true;
				while (download_icon_sleeping != true) {
					usleep(10000);
				}
			}
			fatUnmount("sd:");
			fatUnmount("usb:");
			SYS_ResetSystem(HWButton, 0, 0);
		}
	}

	return 0;
}
