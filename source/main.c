/*

Homebrew Browser - a simple way to view and install Homebrew releases via the Wii

Author: teknecal & Domi04151309
Created: 24/06/2008

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

#include "blank_png.h"
#include "no_image_png.h"

#include "res/drawables.h"
#include "res/res.h"
#include "res/strings.h"
#include "activities.h"
#include "ui.h"
#include "utils.h"

#include "activities/menu.h"
#include "activities/about.h"
#include "activities/help_controller.h"
#include "activities/settings.h"

#define METHOD_SD 1
#define METHOD_USB 2

extern Mtx GXmodelView2D;

// Wiimote IR
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
bool updated_cat = false;
int current_app = 0;

char text_white[10] = " ";
int display_message_counter = 0;

int held_counter = 0;
int cancel_wait = 0;
bool load_updated = false;
int icons_loaded = 0;

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
	if (!state) menu_section = 0;
	refresh_list = -1;
	return state;
}

bool listUpdate = true;
void updateList() {
	listUpdate = true;
}

int main(int argc, char **argv) {
	initialise();
	initialise_reset_button();
	WPAD_SetPowerButtonCallback(WiimotePowerPressed);

	current_time = time(0);
	sprintf(setting_last_boot, "%li", current_time); // bug fix

	GRRLIB_Init();
	GRRLIB_InitFont();
	UI_bootScreen("Loading");

	if (setting_online) initialise_network();

	initialise_fat();
	load_settings();

	if (setting_online && !setting_server) {
		initialise_codemii();
		printf("Attempting to connect to server");
		int main_retries = 0;
		while (!www_passed && main_retries < 3) {
			initialise_www();
			int retries = 0;
			while (!www_passed && retries < 5) {
				sleep(1);
				retries++;
			}
			if (!www_passed) {
				UI_bootScreen("Failed, retrying");
			}
			main_retries++;
			suspend_www_thread();
		}

		if (!www_passed) {
			codemii_backup = true;
			UI_bootScreen("OSCWii appears to be having issues, using OSCWii Backup Server");
			initialise_codemii_backup();
			printf("Attempting to connect to server");

			int main_retries = 0;
			while (!www_passed && main_retries < 3) {
				initialise_www();
				int retries = 0;
				while (!www_passed && retries < 5) {
					sleep(1);
					retries++;
				}
				if (!www_passed) {
					UI_bootScreen("Failed, retrying");
				}
				main_retries++;
				suspend_www_thread();
			}
		}

		suspend_www_thread();

		if (!www_passed) {
			die("Returning you back to HBC. Please check to see if " MAIN_DOMAIN " and " FALLBACK_DOMAIN " are working.");
		}
	}
	else if (setting_server) { // Secondary server setting enabled
		codemii_backup = true;
		initialise_codemii_backup();
		UI_bootScreen("Attempting to connect to OSCWii Secondary server");

		int main_retries = 0;
		while (!www_passed && main_retries < 3) {
			initialise_www();
			int retries = 0;
			while (!www_passed && retries < 5) {
				sleep(1);
				retries++;
			}
			if (!www_passed) {
				UI_bootScreen("Failed, retrying");
			}
			main_retries++;
			suspend_www_thread();
		}

		suspend_www_thread();

		if (!www_passed) {
			die("Returning you back to HBC. Please check to see if " FALLBACK_DOMAIN " is working");
		}
	}
	UI_bootScreen("Connection established");
	repo_check();

	// Grab the homebrew list and parse list
	while (!request_list());

	// Clear homebrew list
	clear_list();

	// List
	category_selection = setting_category;
	if (setting_sort == 0) sort_up_down = 6;
	else if (setting_sort == 1) sort_up_down = 0;

	if (setting_online && setting_repo == 0) {
		check_temp_files();
		update_settings(); // Update last boot time
	}

	suspend_reset_thread();

	// About text
	char str_res_title[100];
	char str_res_size[50];
	char str_res_date[50];

	// About text description
	char string1[80];
	char string2[80];
	char string3[80];
	char string4[80];
	char string5[80];

	char str_sd_card[50];
	char str_icon_info[50];
	char str_download_info[50];

	// Download queue
	char str_title_status[50];

	DRAWABLES_load();

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
	int start_updated = -1;
	int finish_updated = 0;
	int ypos_updated = 184;

	// Main Thread
	ACTIVITIES_open(ACTIVITY_MAIN);
	while(1) {

		WPAD_ScanPads();
		pressed = WPAD_ButtonsDown(0);
		u32 held = WPAD_ButtonsHeld(0);

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
		if (setting_wiiside && !(held & WPAD_BUTTON_1)) {
			if (held & WPAD_BUTTON_RIGHT) {
				ir.y -=5 ;
			} else if (held & WPAD_BUTTON_LEFT) {
				ir.y+=5;
			} else if (held & WPAD_BUTTON_DOWN) {
				ir.x+=5;
			} else if (held & WPAD_BUTTON_UP) {
				ir.x-=5;
			}
		}

		GRRLIB_FillScreen(WINDOW_BACKGROUND);
		UI_roundedRect(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W, UI_PAGE_H, RES_COLOR_WHITE);

		// Changed category
		if (updated_cat) {
			ypos = 142;
			icons_loaded = 0;
			WPAD_Rumble(WPAD_CHAN_0, 0);

			close_windows();
			ACTIVITIES_open(ACTIVITY_MAIN);

			updateList();
			updated_cat = false;
		}

		// Update list
		if (listUpdate) {
			// Tell downloading icon thread to sleep until we've loaded the new category
			if (download_icon > 0 && !download_in_progress) {
				changing_cat = true;
				while (!download_icon_sleeping) usleep(10000);
			}

			if (category_selection != 9) {
				// Clear homebrew list
				clear_list();
			}

			// Update category
			if (category_selection == 0) {
				for (int i = 0; i < array_length(demos_list); i++) {
					current_items[i] = demos_list[i];
				}
			} else if (category_selection == 1) {
				for (int i = 0; i < array_length(emulators_list); i++) {
					current_items[i] = emulators_list[i];
				}
			} else if (category_selection == 2) {
				for (int i = 0; i < array_length(games_list); i++) {
					current_items[i] = games_list[i];
				}
			} else if (category_selection == 3) {
				for (int i = 0; i < array_length(media_list); i++) {
					current_items[i] = media_list[i];
				}
			} else if (category_selection == 4) {
				for (int i = 0; i < array_length(utilities_list); i++) {
					current_items[i] = utilities_list[i];
				}
			} else if (category_selection == 5) {
				int j = 0;
				for (int i = 0; i < array_length(total_list); i++) {
					if (total_list[i].local_app_size > 0) {
						current_items[j] = total_list[i];
						j++;
					}
				}
				wait_a_press = 7;
			} else if (category_selection == 6) {
				int j = 0;
				for (int i = 0; i < array_length(total_list); i++) {
					if (total_list[i].in_download_queue >= 1) {
						current_items[j] = total_list[i];
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

			listUpdate = false;
			refresh_list = -1;
			changing_cat = false;
		}

		int start = -1;
		int finish = 0;
		for (int x = 0; x < array_length(current_items); x++) {
			if (ypos + (76 * x) >= 94 && ypos + (76 * x) + 30 < 400) {
				if (start == -1) start = x;
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
			for (int c = start; c <= (start+4); c++) {
				icons_loaded++;

				if (total_list[current_items[c].original_pos].file_found == 0 || total_list[current_items[c].original_pos].file_found == 2) {
					if (total_list[current_items[c].original_pos].file_found == 0) {
						// Check image file size
						char img_path[100] = "sd:/apps/homebrew_browser_lite/temp/";
						if (!setting_use_sd) strcpy(img_path,"usb:/apps/homebrew_browser_lite/temp/");
						strcat(img_path, total_list[current_items[c].original_pos].name);
						strcat(img_path, ".png");

						FILE *f = fopen(img_path, "rb");

						if (f == NULL) {
							total_list[current_items[c].original_pos].file_found = -1;
						}
						// Open file and check file length for changes
						else {
							fseek (f , 0, SEEK_END);
							int local_img_size = ftell (f);
							rewind(f);
							fclose(f);

							// Load image into memory if image size matches
							if (local_img_size == total_list[current_items[c].original_pos].img_size) {
								total_list[current_items[c].original_pos].file_found = load_file_to_memory(img_path, &total_list[current_items[c].original_pos].content);
							} else {
								total_list[current_items[c].original_pos].file_found = -1;
							}
						}
					}
				}
				if (total_list[current_items[c].original_pos].content != NULL) total_list[current_items[c].original_pos].file_found = 1;

				// Once we've gone through the first 8 images of the category (even if they weren't loaded), we wake the download icon thread and are done
				if (icons_loaded > 8) {
					changing_cat = false;
					icons_loaded = 0;
				}
			}

			// Display images
			if (total_list[current_items[start].original_pos].file_found == 1 && current_items[start].original_pos != -1) icon1_img=GRRLIB_LoadTexture(total_list[current_items[start].original_pos].content);
			else if (strlen(total_list[current_items[start].original_pos].name) >= 3 && current_items[start].original_pos != -1) icon1_img=GRRLIB_LoadTexture(no_image_png);
			else icon1_img=GRRLIB_LoadTexture(blank_png);

			if (total_list[current_items[(start+1)].original_pos].file_found == 1 && current_items[(start+1)].original_pos != -1) icon2_img=GRRLIB_LoadTexture(total_list[current_items[(start+1)].original_pos].content);
			else if (strlen(total_list[current_items[(start+1)].original_pos].name) >= 3 && current_items[(start+1)].original_pos != -1) icon2_img=GRRLIB_LoadTexture(no_image_png);
			else icon2_img=GRRLIB_LoadTexture(blank_png);

			if (total_list[current_items[(start+2)].original_pos].file_found == 1 && current_items[(start+2)].original_pos != -1) icon3_img=GRRLIB_LoadTexture(total_list[current_items[(start+2)].original_pos].content);
			else if (strlen(total_list[current_items[(start+2)].original_pos].name) >= 3 && current_items[(start+2)].original_pos != -1) icon3_img=GRRLIB_LoadTexture(no_image_png);
			else icon3_img=GRRLIB_LoadTexture(blank_png);

			if (total_list[current_items[(start+3)].original_pos].file_found == 1 && current_items[(start+3)].original_pos != -1) icon4_img=GRRLIB_LoadTexture(total_list[current_items[(start+3)].original_pos].content);
			else if (strlen(total_list[current_items[(start+3)].original_pos].name) >= 3 && current_items[(start+3)].original_pos != -1) icon4_img=GRRLIB_LoadTexture(no_image_png);
			else icon4_img=GRRLIB_LoadTexture(blank_png);

			if (strlen(total_list[current_items[(start+4)].original_pos].name) >= 3 && total_list[current_items[(start+4)].original_pos].file_found == 1 && current_items[(start+4)].original_pos != -1) icon5_img=GRRLIB_LoadTexture(total_list[current_items[(start+4)].original_pos].content);
			else if (strlen(total_list[current_items[(start+4)].original_pos].name) >= 3 && current_items[(start+4)].original_pos != -1) icon5_img=GRRLIB_LoadTexture(no_image_png);
			else icon5_img=GRRLIB_LoadTexture(blank_png);

			refresh_list = start;
		}


		// Update ypos from ypos updated
		if (select_repo) ypos = ypos_updated;

		if (ACTIVITIES_current() == ACTIVITY_MAIN) {
			UI_roundedRect(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W, UI_PAGE_H, RES_COLOR_LIGHT_GRAY);
			UI_roundedRect(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W_SMALL, UI_PAGE_H, RES_COLOR_WHITE);

			// Dpad up / down
			if (!setting_wiiside) {
				if (held & WPAD_BUTTON_DOWN && ((strlen(total_list[current_items[finish+1].original_pos].name) > 1) || (select_repo && finish_updated+1 < repo_count))) ypos-= 4;
				else if (held & WPAD_BUTTON_UP && ((!select_repo && ypos <= 140) || (select_repo && ypos <= 180))) ypos+= 4;
			}

			// Wiimote sideways
			if (held & WPAD_BUTTON_1) {
				if (held & WPAD_BUTTON_LEFT && ((strlen(total_list[current_items[finish+1].original_pos].name) > 1) || (select_repo && finish_updated+1 < repo_count))) ypos-= 5;
				else if (held & WPAD_BUTTON_RIGHT && ((!select_repo && ypos <= 140) || (select_repo && ypos <= 180))) ypos+= 5;
			}

			// Update ypos updated from ypos
			if (select_repo) {
				ypos_updated = ypos;
				ypos = 142;
			}

			// Highlighting and display tick, question mark or plus
			GRRLIB_ClipDrawing(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W_SMALL, UI_PAGE_H);
			for (uint8_t b = 0; b < 5; b++) {
				if (strlen(total_list[current_items[(start + b)].original_pos].name) >= 2 && current_items[(start + b)].original_pos != -1) {
					if (UI_isOnSquare(ir, UI_PAGE_X, ypos + (76 * (start + b)), 530, 64)) {
						doRumble = true;
						UI_highlight(UI_PAGE_X, ypos + (76 * (start + b)), UI_PAGE_W_SMALL, 64);

						if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2)) {
							ACTIVITIES_open(ACTIVITY_APP);
							update_about = true;
							current_app = start + b;
							wait_a_press = 10;
						}
						if (pressed & WPAD_BUTTON_PLUS) {
							if (total_list[current_items[(start + b)].original_pos].local_app_size > 0) {
								total_list[current_items[(start + b)].original_pos].in_download_queue = 2;
							} else {
								total_list[current_items[(start + b)].original_pos].in_download_queue = 1;
							}
						} else if (pressed & WPAD_BUTTON_MINUS) {
							total_list[current_items[(start + b)].original_pos].in_download_queue = false;
						}
					}

					if (total_list[current_items[(start + b)].original_pos].local_app_size > 0 && !total_list[current_items[(start + b)].original_pos].in_download_queue) {
						if (total_list[current_items[(start + b)].original_pos].local_app_size == total_list[current_items[(start + b)].original_pos].app_size || total_list[current_items[(start + b)].original_pos].no_manage) {
							GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, app_tick_img, 0, 1, 1, 0xFFFFFFFF);
						} else if (total_list[current_items[(start + b)].original_pos].local_app_size != total_list[current_items[(start + b)].original_pos].app_size) {
							GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, app_question_img, 0, 1, 1, 0xFFFFFFFF);
						}
					}

					if ((total_list[current_items[(start + b)].original_pos].app_time + 432000) > current_time && (strcmp (job_store_list[0].name, total_list[current_items[(start + b)].original_pos].name) != 0)) {
						GRRLIB_DrawImg(468, ypos + (76 * (start + b)) - 6, app_new_img, 0, 1, 1, 0xFFFFFFFF);
					}

					if (total_list[current_items[(start + b)].original_pos].in_download_queue == 1) {
						GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, stack_img, 0, 1, 1, RES_COLOR_GREEN);
					} else if (total_list[current_items[(start + b)].original_pos].in_download_queue == 2) {
						GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, stack_img, 0, 1, 1, RES_COLOR_RED);
					}

					if (strcmp (job_store_list[0].name, total_list[current_items[(start + b)].original_pos].name) == 0 && (download_in_progress || extract_in_progress)) {
						GRRLIB_DrawImg(506, ypos + (76 * (start + b)) + 22, download_img, 0, 1, 1, RES_COLOR_BLUE);
					}
				}
			}

			// Display icons, text
			GRRLIB_DrawImg(60, ypos + (76 * start) + 4, icon1_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(60, ypos + (76 * (start+1) + 4), icon2_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(60, ypos + (76 * (start+2) + 4), icon3_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(60, ypos + (76 * (start+3) + 4), icon4_img, 0, 1, 1, 0xFFFFFFC8);
			GRRLIB_DrawImg(60, ypos + (76 * (start+4) + 4), icon5_img, 0, 1, 1, 0xFFFFFFC8);

			for (uint8_t b = 0; b < 5; b++) {
				GRRLIB_DrawText(210, ypos + (76 * (start + b)) + 4, total_list[current_items[start + b].original_pos].app_name, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY);
				GRRLIB_DrawText(210, ypos + (76 * (start + b)) + 30, total_list[current_items[start + b].original_pos].app_short_description, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);

				// Debigging
				/*char temp[8] = "";
				sprintf(temp, "%i", total_list[current_items[start + b].original_pos);
				GRRLIB_DrawText(210, ypos + (76 * (start + b)) + 30, temp, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);*/
			}

			GRRLIB_ClipReset();

			// Empty messages
			if (category_selection == 6 && current_items[0].original_pos == -1 && wait_a_press == 0) {
				GRRLIB_DrawText(88, 159, STR_EMPTY_QUEUE_0, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 189, STR_EMPTY_QUEUE_1, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 219, STR_EMPTY_QUEUE_2, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 249, STR_EMPTY_QUEUE_3, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
			}
			if (category_selection == 5 && current_items[0].original_pos == -1 && wait_a_press == 0) {
				GRRLIB_DrawText(88, 159, STR_EMPTY_SD_0, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 189, STR_EMPTY_SD_1, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
				GRRLIB_DrawText(88, 219, STR_EMPTY_SD_2, FONTSIZE_SMALL1, TEXT_COLOR_SECONDARY);
			}

			//Sorting icons
			if (UI_isOnImg(ir, UI_SORT_X, UI_SORT_1_Y, name_img)) {
				doRumble = true;
				GRRLIB_DrawImg(UI_SORT_X, UI_SORT_1_Y, name_img, 0, 1, 1, LIGHT_BTN_COLOR_HOVER);
				if (setting_tool_tip) { UI_drawTooltip(UI_SORT_X, UI_SORT_1_Y, STR_SORT_BY_NAME); }
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
					if (sort_up_down == 6) { sort_up_down = 7; sort_by_name(1); refresh_list = -1; ypos = 142; }
					else { sort_up_down = 6; sort_by_name(0); refresh_list = -1; ypos = 142; }
				}
			} else {
				GRRLIB_DrawImg(UI_SORT_X, UI_SORT_1_Y, name_img, 0, 1, 1, LIGHT_BTN_COLOR);
			}
			if (UI_isOnImg(ir, UI_SORT_X, UI_SORT_2_Y, date_img)) {
				doRumble = true;
				GRRLIB_DrawImg(UI_SORT_X, UI_SORT_2_Y, date_img, 0, 1, 1, LIGHT_BTN_COLOR_HOVER);
				if (setting_tool_tip) { UI_drawTooltip(UI_SORT_X, UI_SORT_2_Y, STR_SORT_BY_DATE); }
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
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
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && !download_in_progress) {
					if (category_selection == 6) {
						if (setting_online && hide_apps_updated() && current_items[0].original_pos != -1 && array_length(current_items) >= 1) {
							clear_temp_list();
							updating = 0;
							free_update = true;
							download_queue_size();
						}
					} else {
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
				if ((pressed & WPAD_BUTTON_LEFT) && category_selection > 0 && category_selection < 5 && updating == -1) {
					if ((category_selection-1 == 0 && array_length(demos_list) >= 1) || (category_selection-1 == 1 && array_length(emulators_list) >= 1) || (category_selection-1 == 2 && array_length(games_list) >= 1) || (category_selection-1 == 3 && array_length(media_list) >= 1) || (category_selection-1 == 4 && array_length(utilities_list) >= 1)) {
						category_selection--;
						updated_cat = true;
					}
				}
				if ((pressed & WPAD_BUTTON_RIGHT) && category_selection < 4 && updating == -1) {
					if ((category_selection+1 == 0 && array_length(demos_list) >= 1) || (category_selection+1 == 1 && array_length(emulators_list) >= 1) || (category_selection+1 == 2 && array_length(games_list) >= 1) || (category_selection+1 == 3 && array_length(media_list) >= 1) || (category_selection+1 == 4 && array_length(utilities_list) >= 1)) {
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
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && updating == -1 && array_length(demos_list) >= 1) {
				category_selection = 0;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 1)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && updating == -1 && array_length(emulators_list) >= 1) {
				category_selection = 1;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 2)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && updating == -1 && array_length(games_list) >= 1) {
				category_selection = 2;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 3)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && updating == -1 && array_length(media_list) >= 1) {
				category_selection = 3;
				updated_cat = true;
			}
		} else if (UI_CAT_isOnCategory(ir, 4)) {
			doRumble = true;
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && updating == -1 && array_length(utilities_list) >= 1) {
				category_selection = 4;
				updated_cat = true;
			}
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
			if (setting_tool_tip) { UI_drawTooltip(546, 25, STR_INSTALLED_APPS); }
			if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && updating == -1) {
				category_selection = 5;
				updated_cat = true;
			}
		} else {
			GRRLIB_DrawImg(546, 16, list_img, 0, 1, 1, BTN_COLOR);
		}

		// Updating
		if (updating >= 0 && current_items[0].original_pos != -1) {
			UI_roundedRect(UI_PAGE_X, UI_PAGE_Y, UI_PAGE_W, UI_PAGE_H, RES_COLOR_RED);
			if (free_update) {
				sprintf (str_title_status, "Processing %i/%i applications", new_updating + 1, array_length(current_items));
				strcpy(str_res_title, total_list[current_items[new_updating].original_pos].app_name);
				free_update = false;
			}

			if ((updating < array_length(current_items) && new_updating < array_length(current_items)) || new_updating == 10000) {

				GRRLIB_DrawText(220, 152, str_title_status, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY);
				GRRLIB_DrawText(348 - (strlen(total_list[current_items[updating].original_pos].app_name) * 5.5), 196, str_res_title, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY);

				// Download
				if (download_in_progress) {
					int download_progress_count = (int) download_progress_counter / download_part_size;
					if (download_progress_count > 99) download_progress_count = 100;

					GRRLIB_Rectangle(139, 270, 400, 32, TOOLTIP_COLOR, true);
					GRRLIB_Rectangle(139, 270, download_progress_count * 4, 32, RES_COLOR_BLUE, true);
					GRRLIB_DrawText(208, 276, STR_DOWNLOADING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

					if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) {
						cancel_download = true;
						wait_a_press = 20;
					}

					// Download info
					double temp_download_progress_counter = download_progress_counter;
					double temp_total_app_size = total_list[current_items[updating].original_pos].total_app_size;
					sprintf(str_download_info, "%3.2fMB / %3.2fMB", (temp_download_progress_counter/1000/1000), (temp_total_app_size/1000/1000));
					GRRLIB_DrawText(360, 278, str_download_info, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
				}

				// Extract
				if (extract_in_progress) {

					int extract_progress_count = (int) (zip_progress / extract_part_size);
					if (extract_progress_count > 100) extract_progress_count = 100;

					GRRLIB_Rectangle(139, 270, 400, 32, TOOLTIP_COLOR, true);
					GRRLIB_Rectangle(139, 270, extract_progress_count * 4, 32, RES_COLOR_BLUE, true);
					GRRLIB_DrawText(270, 276, STR_EXTRACTING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

					if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) {
						cancel_extract = true;
						wait_a_press = 20;
					}

					// Extract info
					int temp_extract_file_counter = unzip_file_counter;
					int temp_extract_file_count = unzip_file_count;
					sprintf(str_download_info, "%i / %i", temp_extract_file_counter, temp_extract_file_count);
					GRRLIB_DrawText(410, 278, str_download_info, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
				}

				// Delete in progress
				if (delete_in_progress) {
					GRRLIB_DrawText(290, 276, STR_DELETING, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
					if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) {
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
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
							cancel_confirmed = true;
							new_updating = 10000;
							cancel_wait = 0;
						}
					}

					if (ir.x > 337 && ir.x < 440 && ir.y > 320 && ir.y < 355) {
						doRumble = true;
						GRRLIB_DrawImg(353, 321, button_no_highlight_img, 0, 1, 1, 0xFFFFFFFF);
						if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
							cancel_download = false;
							cancel_extract = false;
							cancel_wait = 0;
						}
					}

					if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) && cancel_wait == -1) {
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
				GRRLIB_DrawText(270, 376, STR_OVERALL_PROGRESS, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);


				if (new_updating != 10000 && (updating != new_updating || updating == 0) && !download_in_progress && !extract_in_progress && !delete_in_progress) {
					updating = new_updating;
					if (strlen(total_list[current_items[updating].original_pos].name) >= 3) {

						// Delete
						if (total_list[current_items[updating].original_pos].in_download_queue == 2) {
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
							if (total_list[current_items[selected_app].original_pos].local_app_size > 0 && total_list[current_items[selected_app].original_pos].local_app_size != total_list[current_items[selected_app].original_pos].app_size) {
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

					for (int x = 0; x < array_length(current_items); x++) {
						temp_list2[x] = total_list[current_items[x].original_pos];
					}

					clear_list();

					for (uint8_t i = 0; i < 4; i++) {
						current_items[i].original_pos = -1;
					}

					int j = 0;
					for (int i = 0; i < array_length(temp_list2); i++) {
						total_list[current_items[j].original_pos] = temp_list2[i];
						j++;
					}

					category_selection = 9;
					updated_cat = true;
					refresh_list = -1;
				}
			}

			// Download failed or extracting failed?
			if (download_in_progress == -1 || extract_in_progress == -1 || delete_in_progress == -1) {

				// Display what error occured
				if (error_number == 1) GRRLIB_DrawText(210, 276, STR_DOWNLOAD_ZIP_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 2) GRRLIB_DrawText(210, 276, STR_CREATE_FOLDER_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 3) GRRLIB_DrawText(210, 276, STR_DOWNLOAD_ICON_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 4) GRRLIB_DrawText(210, 276, STR_EXTRACT_ZIP_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 5) GRRLIB_DrawText(190, 276, STR_BOOT_FILE_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 6) GRRLIB_DrawText(210, 396, STR_DELETE_FILE_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 9) GRRLIB_DrawText(240, 276, STR_FREE_SPACE_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 10) GRRLIB_DrawText(240, 276, STR_NO_WIFI, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);

				// Reset vars
				if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) && wait_a_press == 0) {
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

			if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) && !download_in_progress && !extract_in_progress && display_message_counter == 0 && wait_a_press == 0) {
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
				if (total_list[current_items[current_app].original_pos].app_total_size > 0 && total_list[current_items[current_app].original_pos].app_total_size < 1048576) {
					int appsize = total_list[current_items[current_app].original_pos].app_total_size / 1024;
					sprintf(str_res_size, "%i KB", appsize);
				} else {
					float appsize = (float) (total_list[current_items[current_app].original_pos].app_total_size / 1024) / 1024;
					sprintf(str_res_size, "%1.1f MB", appsize);
				}

				app_time = total_list[current_items[current_app].original_pos].app_time;
				timeinfo = localtime( &app_time );
				strftime(str_res_date, 50, "%d %b %Y", timeinfo);

				const int text_size = sizeof(total_list[current_items[0].original_pos].app_description);
				char text_description[text_size];
				strcpy(text_description, total_list[current_items[current_app].original_pos].app_description);

				int l = 0;
				for(int s = strlen(text_description); s < text_size; s++) {
					text_description[s] = text_white[0];
					l = text_size > s ? s : text_size;
				}
				text_description[l] = '\0';

				int count = 0;
				int textrow = 0;
				int offset = 0;
				char test[80];
				for (int x = 0; x < strlen(text_description); x++) {
					test[count] = text_description[x];
					count++;
					if (x >= (55 * (textrow+1)) && x <= (75 * (textrow+1)) && text_description[x] == ' ' && textrow == 0) {
						test[count] = '\0';
						strcpy(string1, test);
						textrow = 1;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 1) {
						test[count] = '\0';
						strcpy(string2, test);
						textrow = 2;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 2) {
						test[count] = '\0';
						strcpy(string3, test);
						textrow = 3;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 3) {
						test[count] = '\0';
						strcpy(string4, test);
						textrow = 4;
						offset+= count - 55;
						count = 0;
					}
					if (x >= (55 * (textrow+1) + offset) && x <= (75 * (textrow+1) + offset) && text_description[x] == ' ' && textrow == 4) {
						test[count] = '\0';
						strcpy(string5, test);
						count = 0;
						textrow = 5;
						break;
					}
				}

				update_about = false;
			}

			GRRLIB_DrawText(330 - (strlen(total_list[current_items[current_app].original_pos].app_name) * 5.5), 140, total_list[current_items[current_app].original_pos].app_name, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY);

			GRRLIB_DrawText(70, 170, string1, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
			GRRLIB_DrawText(70, 190, string2, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
			GRRLIB_DrawText(70, 210, string3, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
			GRRLIB_DrawText(70, 230, string4, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
			GRRLIB_DrawText(70, 250, string5, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);

			GRRLIB_DrawText(70, 290, STR_AUTHOR, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(70, 310, STR_VERSION, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(70, 330, STR_SIZE, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(70, 350, STR_DATE, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);

			GRRLIB_DrawText(140, 290, total_list[current_items[current_app].original_pos].app_author, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
			GRRLIB_DrawText(140, 310, total_list[current_items[current_app].original_pos].app_version, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
			GRRLIB_DrawText(140, 330, str_res_date, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);
			GRRLIB_DrawText(140, 350, str_res_size, FONTSIZE_SMALLER, TEXT_COLOR_SECONDARY);

			if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "wwww")) GRRLIB_DrawImg(290, 330, control_wiimote_4_img, 0, 1, 1, 0xFFFFFFFF);
			else if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "www")) GRRLIB_DrawImg(290, 330, control_wiimote_3_img, 0, 1, 1, 0xFFFFFFFF);
			else if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "ww")) GRRLIB_DrawImg(290, 330, control_wiimote_2_img, 0, 1, 1, 0xFFFFFFFF);
			else if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "w")) GRRLIB_DrawImg(290, 330, control_wiimote_img, 0, 1, 1, 0xFFFFFFFF);
			else GRRLIB_DrawImg(290, 330, control_wiimote_img, 0, 1, 1, 0xFFFFFF3C);

			if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "n")) GRRLIB_DrawImg(310, 330, control_nunchuck_img, 0, 1, 1, 0xFFFFFFFF);
			else GRRLIB_DrawImg(310, 330, control_nunchuck_img, 0, 1, 1, 0xFFFFFF3C);

			if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "c")) GRRLIB_DrawImg(346, 330, control_classic_img, 0, 1, 1, 0xFFFFFFFF);
			else GRRLIB_DrawImg(346, 330, control_classic_img, 0, 1, 1, 0xFFFFFF3C);

			if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "g")) GRRLIB_DrawImg(395, 330, control_gcn_img, 0, 1, 1, 0xFFFFFFFF);
			else GRRLIB_DrawImg(395, 330, control_gcn_img, 0, 1, 1, 0xFFFFFF3C);

			if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "z")) GRRLIB_DrawImg(455, 330, control_zapper_img, 0, 1, 1, 0xFFFFFFFF);
			else GRRLIB_DrawImg(455, 330, control_zapper_img, 0, 1, 1, 0xFFFFFF3C);

			if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "k")) GRRLIB_DrawImg(544, 330, control_keyboard_img, 0, 1, 1, 0xFFFFFFFF);
			else GRRLIB_DrawImg(544, 330, control_keyboard_img, 0, 1, 1, 0xFFFFFF3C);

			GRRLIB_DrawText(489, 290, STR_SDHC, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			if (strstr(total_list[current_items[current_app].original_pos].app_controllers, "s")) GRRLIB_DrawText(553, 290, STR_YES, FONTSIZE_SMALLER, RES_COLOR_GREEN);
			else GRRLIB_DrawText(553, 290, STR_NO, FONTSIZE_SMALLER, RES_COLOR_RED);

			if ((!download_in_progress && !extract_in_progress && !delete_in_progress) || (strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) != 0)) {
				// Download or updated enabled?
				if (total_list[current_items[current_app].original_pos].local_app_size > 0) {
					if (total_list[current_items[current_app].original_pos].local_app_size != total_list[current_items[current_app].original_pos].app_size && download_arrow == 0 && !total_list[current_items[current_app].original_pos].no_manage) {
						if (UI_button(ir, 340, 385, STR_UPDATE)) {
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
								download_in_progress = true;
								selected_app = current_app;
								add_to_stats();
								save_xml_name();
								initialise_download();
								if (total_list[current_items[selected_app].original_pos].local_app_size > 0 && total_list[current_items[selected_app].original_pos].local_app_size != total_list[current_items[selected_app].original_pos].app_size) {
									update_xml = 1;
								}
							}
						}
					}
					else if (total_list[current_items[current_app].original_pos].local_app_size > 0 && download_arrow == 0) {
						UI_drawButton(340, 385, STR_UPDATE, 2);
					}

					if (download_arrow == 0) {
						if (UI_button(ir, 485, 385, STR_DELETE)) {
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
								delete_in_progress = true;
								selected_app = current_app;
								initialise_delete();
							}
						}
					}

					if (download_arrow != 0) {
						if (!total_list[current_items[current_app].original_pos].no_manage) {
							GRRLIB_DrawImg(490, 385, button_yes_img, 0, 1, 1, 0xFFFFFF96);

							if (ir.x > 480 && ir.x < 580 && ir.y > 385 && ir.y < 420) {
								doRumble = true;
								GRRLIB_DrawImg(490, 385, button_yes_img, 0, 1, 1, 0xFFFFFFFF);
								if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
									total_list[current_items[current_app].original_pos].no_manage = true;
								}
							}
						}
						else {
							GRRLIB_DrawImg(490, 385, button_no_img, 0, 1, 1, 0xFFFFFF96);

							if (ir.x > 480 && ir.x < 580 && ir.y > 385 && ir.y < 420) {
								doRumble = true;
								GRRLIB_DrawImg(490, 385, button_no_img, 0, 1, 1, 0xFFFFFFFF);
								if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
									total_list[current_items[current_app].original_pos].no_manage = false;
								}
							}
						}
					}
				}
				else {
					if (download_in_progress && strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) != 0) {
						UI_drawButton(340, 385, STR_DOWNLOAD, 2);
					} else {
						if (UI_button(ir, 340, 385, STR_DOWNLOAD)) {
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
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
			if (download_in_progress && strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) == 0) {

				int download_progress_count = (int) download_progress_counter / download_part_size;
				if (download_progress_count > 99) download_progress_count = 100;

				GRRLIB_Rectangle(139, 390, 400, 32, TOOLTIP_COLOR, true);
				GRRLIB_Rectangle(139, 390, download_progress_count * 4, 32, RES_COLOR_BLUE, true);
				GRRLIB_DrawText(208, 396, STR_DOWNLOADING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

				if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) {
					cancel_download = true;
				}

				// Download info
				double temp_download_progress_counter = download_progress_counter;
				double temp_total_app_size = job_store_list[0].total_app_size;
				sprintf(str_download_info, "%3.2fMB / %3.2fMB", (temp_download_progress_counter/1000/1000), (temp_total_app_size/1000/1000));
				GRRLIB_DrawText(360, 398, str_download_info, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
			}

			// Extracting in progress, display progress
			if (extract_in_progress && strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) == 0) {

				int extract_progress_count = (int) (zip_progress / extract_part_size);
				if (extract_progress_count > 100) extract_progress_count = 100;

				GRRLIB_Rectangle(139, 390, 400, 32, TOOLTIP_COLOR, true);
				GRRLIB_Rectangle(139, 390, extract_progress_count * 4, 32, RES_COLOR_BLUE, true);
				GRRLIB_DrawText(270, 396, STR_EXTRACTING, FONTSIZE_SMALL, TEXT_COLOR_PRIMARY_DARK);

				if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) {
					cancel_extract = true;
				}

				// Extract info
				int temp_extract_file_counter = unzip_file_counter;
				int temp_extract_file_count = unzip_file_count;
				sprintf(str_download_info, "%i / %i", temp_extract_file_counter, temp_extract_file_count);
				GRRLIB_DrawText(410, 398, str_download_info, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);
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
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
						cancel_confirmed = true;
						cancel_wait = 0;
					}
				}

				if (ir.x > 337 && ir.x < 440 && ir.y > 320 && ir.y < 355) {
					doRumble = true;
					GRRLIB_DrawImg(353, 321, button_no_highlight_img, 0, 1, 1, 0xFFFFFFFF);
					if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
						cancel_download = false;
						cancel_extract = false;
						cancel_wait = 0;
					}
				}

				if ((pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) && cancel_wait == -1) {
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
			if (delete_in_progress && strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) == 0) {
				GRRLIB_DrawText(270, 396, STR_DELETING, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
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
			if (display_message_counter > 0 && strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) == 0) {
				display_message_counter--;

				// Display what error occured
				if (error_number == 1) GRRLIB_DrawText(190, 396, STR_DOWNLOAD_ZIP_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 2) GRRLIB_DrawText(190, 396, STR_CREATE_FOLDER_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 3) GRRLIB_DrawText(190, 396, STR_DOWNLOAD_ICON_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 4) GRRLIB_DrawText(190, 396, STR_EXTRACT_ZIP_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 5) GRRLIB_DrawText(190, 396, STR_BOOT_FILE_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 6) GRRLIB_DrawText(190, 396, STR_DELETE_FILE_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 7) GRRLIB_DrawText(210, 396, STR_DELETE_FOLDER_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 8) GRRLIB_DrawText(190, 396, STR_DELETE_APP_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 9) GRRLIB_DrawText(190, 396, STR_FREE_SPACE_FAILED, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);
				else if (error_number == 10) GRRLIB_DrawText(190, 396, STR_NO_WIFI, FONTSIZE_SMALL, TEXT_COLOR_SECONDARY);

				if (pressed & WPAD_BUTTON_B || pressed & WPAD_BUTTON_1) {
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
		if (ACTIVITIES_current() == ACTIVITY_MENU) MENU_render();
		// About
		else if (ACTIVITIES_current() == ACTIVITY_ABOUT) ABOUT_render();
		// Help Controller
		else if (ACTIVITIES_current() == ACTIVITY_HELP_CONTROLLER) HELP_CONTROLLER_render();
		// Settings
		else if (ACTIVITIES_current() == ACTIVITY_SETTINGS) SETTINGS_render();

		// Home button press
		if (pressed & WPAD_BUTTON_HOME && updating == -1 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
			if (ACTIVITIES_current() == ACTIVITY_MENU) ACTIVITIES_goBack();
			else {
				close_windows();
				ACTIVITIES_open(ACTIVITY_MENU);
			}
			wait_a_press = 10;
		}

		// B button press
		if (pressed & WPAD_BUTTON_B && updating == -1 && !download_in_progress && !extract_in_progress && !delete_in_progress) {
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


//TODO: Move to settings
		// Select Repo
		if (select_repo) {
			GRRLIB_DrawImg(123, 148, apps_repo_img, 0, 1, 1, 0xFFFFFFFF);

			start_updated = -1;
			for (int x = 0; x < repo_count; x++) {
				if (ypos_updated + (25 * x) >= 175 && ypos_updated + (25 * x) + 30 < 425) {
					if (start_updated == -1) {
						start_updated = x;
					}
					if (start_updated >= 0) {

						if (UI_isOnSquare(ir, 150, ypos_updated + (25 * x) - 1, 328, 24)) {
							doRumble = true;
							UI_highlight(150, ypos_updated + (25 * x) - 1, 328, 24);
							if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
								setting_repo = x;
							}
						}

						GRRLIB_DrawText(150, (x * 25) + ypos_updated, repo_list[x].name, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY);

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
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
					select_repo = false;
					wait_a_press = 10;
				}
			}
		}

		// Select category
		if (select_category) {
			GRRLIB_DrawImg(123, 148, apps_start_cat_img, 0, 1, 1, 0xFFFFFFFF);

			GRRLIB_DrawText(180, 195, STR_DEMOS, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(180, 225, STR_EMULATORS, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(180, 255, STR_GAMES, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(180, 285, STR_MEDIA, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(180, 315, STR_UTILITIES, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);

			if (UI_isOnSquare(ir, 165, 197, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 197, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
					setting_category = 0;
				}
			}

			if (UI_isOnSquare(ir, 165, 227, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 227, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
					setting_category = 1;
				}
			}

			if (UI_isOnSquare(ir, 165, 257, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 257, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
					setting_category = 2;
				}
			}

			if (UI_isOnSquare(ir, 165, 287, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 287, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
					setting_category = 3;
				}
			}

			if (UI_isOnSquare(ir, 165, 317, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 317, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
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
				if (pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) {
					select_category = false;
					wait_a_press = 10;
				}
			}
		}

		// Select sort
		if (select_sort) {
			GRRLIB_DrawImg(123, 148, apps_start_sort_img, 0, 1, 1, 0xFFFFFFFF);

			GRRLIB_DrawText(180, 195, STR_SORT_BY_NAME, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);
			GRRLIB_DrawText(180, 225, STR_SORT_BY_DATE, FONTSIZE_SMALL1, TEXT_COLOR_PRIMARY);

			if (UI_isOnSquare(ir, 165, 197, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 197, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
					setting_sort = 0;
				}
			}

			if (UI_isOnSquare(ir, 165, 227, 224, 24)) {
				doRumble = true;
				UI_highlight(165, 227, 224, 24);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
					setting_sort = 1;
				}
			}

			if (setting_sort == 0) GRRLIB_DrawImg(380, 198, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);
			else if (setting_sort == 1) GRRLIB_DrawImg(380, 228, app_tick_small_img, 0, 1, 1, 0xFFFFFFFF);

			GRRLIB_DrawImg(386, 156, updated_close_img, 0, 1, 1, 0xFFFFFFFF);

			if (ir.x > 372 && ir.x < 420 && ir.y > 155 && ir.y < 173) {
				doRumble = true;
				GRRLIB_DrawImg(386, 156, updated_close_highlight_img, 0, 1, 1, 0xFFFFFFFF);
				if ((pressed & WPAD_BUTTON_A || pressed & WPAD_BUTTON_2) && wait_a_press == 0) {
					select_sort = false;
					wait_a_press = 10;
				}
			}
		}
// END SETTINGS


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

			sprintf(str_sd_card, "%lli MB Free", sd_card_free);
			sd_card_update = false;
		}

		if (setting_sd_card) GRRLIB_DrawText(468, UI_BOTTOM_TEXT_Y, str_sd_card, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY_DARK);

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

		// Small bottom text
		if (download_icon > 0 && !download_in_progress && !extract_in_progress) {
			sprintf(str_icon_info, "Checking icon %i / %i", download_icon, array_length(total_list));
			GRRLIB_DrawText(248, UI_BOTTOM_TEXT_Y, str_icon_info, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY_DARK);
		}
		if (download_in_progress && updating == -1 && strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) != 0) {
			GRRLIB_DrawText(248, UI_BOTTOM_TEXT_Y, STR_DOWNLOADING_SMALL, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY_DARK);
		}
		if (extract_in_progress && updating == -1 && strcmp (job_store_list[0].name, total_list[current_items[current_app].original_pos].name) != 0) {
			GRRLIB_DrawText(248, UI_BOTTOM_TEXT_Y, STR_EXTRACTING_SMALL, FONTSIZE_SMALLER, TEXT_COLOR_PRIMARY_DARK);
		}

		// IR pointer
		GRRLIB_DrawImg(ir.x - (*mouse_img).w / 2, ir.y - (*mouse_img).h / 2, mouse_img, ir.angle, 1, 1, 0xFFFFFFFF);

		GRRLIB_Render();

		// Exit
		if ((held & WPAD_BUTTON_HOME) && held_counter >= 1) {
			held_counter++;
		}
		if (pressed & WPAD_BUTTON_HOME) {
			held_counter = 1;
		}
		if (held_counter > 20) {
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

		if (HWButton != -1) {
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
			SYS_ResetSystem(HWButton, 0, 0);
		}
	}
	return 0;
}
