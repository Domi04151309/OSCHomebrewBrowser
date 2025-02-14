/*

Homebrew Browser - a simple way to view and install Homebrew releases via the Wii

Author: teknecal & Domi04151309

Using some source from ftpii v0.0.5
ftpii Source Code Copyright (C) 2008 Joseph Jordan <joe.ftpii@psychlaw.com.au>

*/
#include <errno.h>
#include <fat.h>
#include <math.h>
#include <network.h>
#include <asndlib.h>
#include <ogc/lwp_watchdog.h>
#include <ogcsys.h>
#include <ogc/pad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <zlib.h>
#include <mxml.h>
#include "unzip/unzip.h"
#include "unzip/miniunz.h"
#include "GRRLIB/GRRLIB.h"
#include "common.h"
#include "dns.h"

// Modified Tantric FAT code
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>

#include <sys/param.h>

#include "activities.h"
#include "ui.h"
#include "res/res.h"

char rootdir[10];

const DISC_INTERFACE* sd = &__io_wiisd;
const DISC_INTERFACE* usb = &__io_usbstorage;

#define METHOD_SD 1
#define METHOD_USB 2

#define FONTSIZE_SMALL 18
#define BUFFER_SIZE 1024
#define NET_BUFFER_SIZE 1024
#define IP_ADDRESS_OLD "79.136.53.163"
#define IP_ADDRESS_OLD2 "69.163.186.246"
#define SOCKET_PORT 80
u32 IP_ADDRESS = 0;

const char *CRLF = "\r\n";
const u32 CRLF_LENGTH = 2;

static GXRModeObj *vmode;
int xfb_height = 0;

struct repo_struct repo_list[200];

// List to show
int current_items[HOMEBREW_STRUCT_SIZE];

int emulators_list[HOMEBREW_STRUCT_SIZE];
int games_list[HOMEBREW_STRUCT_SIZE];
int media_list[HOMEBREW_STRUCT_SIZE];
int utilities_list[HOMEBREW_STRUCT_SIZE];
int demos_list[HOMEBREW_STRUCT_SIZE];

// Total list
struct homebrew_struct total_list[HOMEBREW_STRUCT_SIZE];

// Temp list
int temp_list[HOMEBREW_STRUCT_SIZE];

// Temp list to use to download/extract/delete
struct homebrew_struct job_store;

// Folders exist list
static char folder_list[HOMEBREW_STRUCT_SIZE][100];

static volatile u8 reset = 0;
static lwp_t reset_thread;
static lwp_t icons_thread;
static lwp_t download_thread;
static lwp_t delete_thread;
static lwp_t request_thread;
static lwp_t www_thread;

int category_old_selection = 2;

long remote_hb_size = 0;
char update_text[1000];

bool hostname_ok = true;
bool codemii_backup = false;
bool www_passed = false;

int download_in_progress = 0;
int extract_in_progress = 0;
int delete_in_progress = 0;
int selected_app = 0;
int total_list_count = 0;

int repo_count = 0;
int timeout_counter = 0;

bool download_icon_sleeping = false;

int download_part_size = 0;
long download_progress_counter = 0;

int error_number = 0;
bool cancel_download = false;
bool cancel_delete = false;

bool sd_card_update = true;
long long sd_card_free = 0;
int sd_free = 0;

int load_icon = 0;
int download_icon = 0;

int sort_up_down = 0;

bool setting_sd_card = true;
bool setting_hide_installed = false;
bool setting_online = true;
bool setting_rumble = true;
bool setting_update_icon = false;
bool setting_tool_tip = true;
char setting_last_boot[14] = "1";
bool setting_use_sd = true;
int setting_repo = 0;
int setting_category = 2;
int setting_sort = 1;
bool setting_disusb = false;
bool setting_wiiside = false;

bool setting_repo_revert = false;
bool cancel_confirmed = false;

int updating = -1;
int new_updating = 0;
int updating_total_size = 0;
int updating_part_size = 0;
long updating_current_size = 0;

int progress_number = 0;
int progress_size = 0;
bool changing_cat = false;
int retry = 0;
int update_xml = 0;

char temp_name[100];
char uppername[100];

int hbb_string_len = 0;
int hbb_null_len = 0;
bool sd_mounted = false;
bool usb_mounted = false;

bool list_received = false;
int no_manage_count = 0;

// Directory vars
char filename[MAXPATHLEN];
char foldername[MAXPATHLEN];
DIR* dir;
struct dirent *dent;
struct stat st;

// Probably used somewhere TODO: Check where
char * buffer;
size_t result;

// Time info
struct tm * timeinfo;
time_t app_time;

// Zip
int zip_size = 0;
long extract_part_size = 0;
long zip_progress = 0;
bool cancel_extract = false;
bool hbb_updating = false;
int unzip_file_counter = 0;
int unzip_file_count = 0;
char no_unzip_list[10][300];
int no_unzip_count = 0;

void exitApp(int code) {
	GRRLIB_Exit();
	exit(code);
}

char* get_error_msg(s32 error_code) {
	switch (error_code) {
		case -6:
			return "Are you connected to the internet?\nTry running a connection test in the Wii System Settings.";
		case -81:
			return "Is your SD card write-locked?\nIs the server in settings.xml not set to 0?";
		default:
			return "Undocumented error code.";
	}
}

static void reset_called() {
	reset = 1;
}

static void *run_reset_thread(void *arg) {
	while (!reset && !(WPAD_ButtonsHeld(0) & WPAD_BUTTON_HOME) && !(PAD_ButtonsDown(0) & PAD_BUTTON_START) ) {
		sleep(1);
		WPAD_ScanPads();
		// Go offline
		if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_1 || PAD_ButtonsDown(0) & PAD_BUTTON_Y) {
			setting_online = false;
			printf("\nNow working in offline mode.\n");
		}
		if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_2 || PAD_ButtonsDown(0) & PAD_BUTTON_X) {
			setting_repo_revert = true;
			setting_repo = 0;
			printf("\nReverting to OSCWii.org repository.\n");
		}
		if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_B || PAD_ButtonsDown(0) & PAD_BUTTON_B) {
			cancel_download = true;
			cancel_extract = true;
		}
	}
	printf("\nHomebrew Browser shutting down...\n");
	exitApp(0);
	return 0;
}

u8 initialise_reset_button() {
	s32 result = LWP_CreateThread(&reset_thread, run_reset_thread, NULL, NULL, 0, 80);
	if (result == 0) SYS_SetResetCallback(reset_called);
	return !result;
}

static void *run_www_thread(void *arg) {
	s32 main_server = server_connect(1);
	net_close(main_server);
	www_passed = true;
	return 0;
}

u8 initialise_www() {
	s32 result = LWP_CreateThread(&www_thread, run_www_thread, NULL, NULL, 0, 80);
	return result;
}

void suspend_www_thread() {
	LWP_SuspendThread(www_thread);
}

static void *run_icons_thread(void *arg) {
	sleep(5);

	int x;
	for (x = 0; x < array_length(total_list); x++) {

		while (changing_cat) {
			download_icon_sleeping = true;
			sleep(3);
		}

		while (download_in_progress || extract_in_progress || delete_in_progress || ACTIVITIES_current() == ACTIVITY_MENU) {
			download_icon_sleeping = true;
			sleep(3);
		}

		download_icon_sleeping = false;

		bool update_img_file = false;

		char img_path[150];
		strcpy(img_path, rootdir);
		strcat(img_path, "apps/homebrew_browser_lite/temp/");
		strcat(img_path, total_list[x].name);
		strcat(img_path, ".png");

		FILE *f = fopen(img_path, "rb");

		// If file doesn't exist or can't open it then we can grab the latest file
		if (f == NULL) {
			update_img_file = true;
		}
		// Open file and check file length for changes
		else {
			fseek (f , 0, SEEK_END);
			int local_img_size = ftell (f);
			rewind(f);
			fclose(f);

			// Check if remote image size doesn't match local image size
			if (local_img_size != total_list[x].img_size) {
				update_img_file = true;
			}
			else {
				// Load image into memory if image size matches
				if (local_img_size == total_list[x].img_size) {
					total_list[x].file_found = load_file_to_memory(img_path, &total_list[x].content);
					if (total_list[x].file_found == 1) {
						total_list[x].file_found = 2;
					}
				}
				else {
					total_list[x].file_found = -1;
				}
			}
		}

		// Grab the new img file
		if (update_img_file && setting_online) {
			s32 create_result;

			char temp_path[200];
			strcpy(temp_path, rootdir);
			strcat(temp_path, "apps/homebrew_browser_lite/temp/");

			create_result = create_and_request_file(temp_path, total_list[x].name, ".png");

			if (create_result == 1) {

				// Make sure that image file matches the correct size
				f = fopen(img_path, "rb");

				// If file doesn't exist or can't open it then we can grab the latest file
				if (f == NULL) {
					update_img_file = true;
				}
				// Open file and check file length for changes
				else {
					fseek (f , 0, SEEK_END);
					int local_img_size = ftell (f);
					rewind(f);
					fclose(f);

					if (local_img_size == total_list[x].img_size) {
						total_list[x].file_found = load_file_to_memory(img_path, &total_list[x].content);
						if (total_list[x].file_found == 1) {
							total_list[x].file_found = 2;
						}
					}
					else {
						total_list[x].file_found = -1;
					}
				}
			}
		}

		// Sleep longer if downloaded image file
		if (update_img_file) {
			sleep(2);
		}
		download_icon = x;
	}
	download_icon = 0;
	return 0;
}

u8 load_icons() {
	s32 result = LWP_CreateThread(&icons_thread, run_icons_thread, NULL, NULL, 0, 80);
	return result;
}


static void *run_download_thread(void *arg) {

	clear_store_list();
	job_store = total_list[current_items[selected_app]];

	if (download_icon > 0) {
		while (download_icon_sleeping != true) {
			usleep(10000);
		}
	}

	bool download_status = true;
	download_progress_counter = 0;

	// Check if there is enough space on SD card
	int check_size = ((job_store.total_app_size / 1024) / 1024) + ((job_store.app_total_size / 1024) / 1024);
	if (sd_card_free <= check_size) {
		download_status = false;
		error_number = 9;
	}

	// Check we are still connected to the Wireless
	if (!check_wifi()) {
		download_status = false;
		error_number = 10;
	}

	if (download_status) {
		download_part_size = (int) (job_store.total_app_size / 100);

		// Download zip file
		char zipfile[512];
		strcpy(zipfile, "/");
		strcat(zipfile, job_store.name);
		strcat(zipfile, ".zip");

		char temp_path[200];
		strcpy(temp_path, rootdir);
		strcat(temp_path, "apps/");

		if (strcmp(job_store.name,"ftpii") == 0) {

			if (create_and_request_file(temp_path, job_store.user_dirname, zipfile) != 1) {
				download_status = false;
				error_number = 1;
			}

		}
		else {
			if (create_and_request_file(temp_path, job_store.name, zipfile) != 1) {
				download_status = false;
				error_number = 1;
			}
		}
	}

	// Directories to create
	if (job_store.folders != NULL && download_status) {
		char folders[1000];
		strcpy(folders, job_store.folders);

		char *split_tok;
		split_tok = strtok (folders,";");

		while (split_tok != NULL && download_status) {
			char temp_create[150] = "sd:";
			if (!setting_use_sd) {
				strcpy(temp_create,"usb:");
			}
			strcat(temp_create, split_tok);
			if (create_dir(temp_create) != 1) {
				download_status = false;
				error_number = 2;
			}
			split_tok = strtok (NULL, ";");
		}
	}

	// Download the icon.png
	if (download_status) {

		char icon_path[100] = "sd:/apps/";
		if (!setting_use_sd) {
			strcpy(icon_path,"usb:/apps/");
		}

		// Only download the icon file if user hasn't got this app installed or doesn't have the icon.png
		if (strcmp(job_store.name,"ftpii") == 0) {
			strcat(icon_path, job_store.user_dirname);
		}
		else {
			strcat(icon_path, job_store.name);
		}
		strcat(icon_path, "/icon.png");

		FILE *f = fopen(icon_path, "rb");

		// Problems opening the file? Then download the icon.png
		if (f == NULL) {

			if (strcmp(job_store.name,"ftpii") == 0) {
				if (setting_use_sd) {
					if (create_and_request_file("sd:/apps/", job_store.user_dirname, "/icon.png") != 1) {
						download_status = false;
						error_number = 3;
					}
				}
				else {
					if (create_and_request_file("usb:/apps/", job_store.user_dirname, "/icon.png") != 1) {
						download_status = false;
						error_number = 3;
					}
				}
			}
			else {
				if (setting_use_sd) {
					if (create_and_request_file("sd:/apps/", job_store.name, "/icon.png") != 1) {
						download_status = false;
						error_number = 3;
					}
				}
				else {
					if (create_and_request_file("usb:/apps/", job_store.name, "/icon.png") != 1) {
						download_status = false;
						error_number = 3;
					}
				}
			}

			usleep(150000);

		}
		else {
			fclose(f);

			if (setting_update_icon) {
				if (strcmp(job_store.name,"ftpii") == 0) {
					if (setting_use_sd) {
						if (create_and_request_file("sd:/apps/", job_store.user_dirname, "/icon.png") != 1) {
							download_status = false;
							error_number = 3;
						}
					}
					else {
						if (create_and_request_file("usb:/apps/", job_store.user_dirname, "/icon.png") != 1) {
							download_status = false;
							error_number = 3;
						}
					}
				}
				else {
					if (setting_use_sd) {
						if (create_and_request_file("sd:/apps/", job_store.name, "/icon.png") != 1) {
							download_status = false;
							error_number = 3;
						}
					}
					else {
						if (create_and_request_file("usb:/apps/", job_store.name, "/icon.png") != 1) {
							download_status = false;
							error_number = 3;
						}
					}
				}
			}

			usleep(150000);
		}
	}

	// Failed or success?
	if (!download_status) {
		download_in_progress = -1;
		download_progress_counter = 0;

		// Remove incomplete zip file
		char extractzipfile[512];
		strcpy(extractzipfile, "sd:/apps/");
		if (!setting_use_sd) {
			strcpy(extractzipfile, "usb:/apps/");
		}
		if (strcmp(job_store.name,"ftpii") == 0) {
			strcat(extractzipfile, job_store.user_dirname);
		}
		else {
			strcat(extractzipfile, job_store.name);
		}

		strcat(extractzipfile, "/");
		strcat(extractzipfile, job_store.name);
		strcat(extractzipfile, ".zip");

		remove_file(extractzipfile);

		if (ACTIVITIES_current() != ACTIVITY_APP && updating == -1) {
			download_in_progress = false;
			extract_in_progress = false;
			delete_in_progress = false;
			cancel_download = false;
			cancel_extract = false;
		}

		return 0;
	}
	else {
		extract_in_progress = true;
		download_in_progress = false;
		download_progress_counter = 0;
	}

	bool extract_status = true;

	// Delete boot.elf file
	char del_file[150] = "sd:/apps/";
	if (!setting_use_sd) {
		strcpy(del_file,"usb:/apps/");
	}
	strcat(del_file, job_store.name);
	strcat(del_file, "/boot.elf");
	remove_file(del_file);

	// Delete boot.dol file or theme.zip
	char del_file1[150] = "sd:/apps/";
	char del_file2[150];
	if (!setting_use_sd) {
		strcpy(del_file1,"usb:/apps/");
	}
	//strcat(del_file1, job_store.name);
	if (strcmp(job_store.name,"ftpii") == 0) {
		strcat(del_file1, job_store.user_dirname);
	}
	else {
		strcat(del_file1, job_store.name);
	}
	strcpy(del_file2, del_file1);
	strcat(del_file1, "/boot.dol");
	strcat(del_file2, "/theme.zip");
	remove_file(del_file1);
	remove_file(del_file2);


	// Put files not to extract in array
	no_unzip_count = 0;

	char folders[300];
	strcpy(folders, job_store.files_no_extract);
	char *split_tok;
	split_tok = strtok (folders,";");

	while (split_tok != NULL) {
		strcpy(no_unzip_list[no_unzip_count], split_tok);
		no_unzip_count++;
		split_tok = strtok (NULL, ";");
	}

	int extract_attempt = 0;
	char extractzipfile[512];
	strcpy(extractzipfile, "sd:/apps/");
	if (!setting_use_sd) {
		strcpy(extractzipfile, "usb:/apps/");
	}
	//strcat(extractzipfile, job_store.name);
	if (strcmp(job_store.name,"ftpii") == 0) {
		strcat(extractzipfile, job_store.user_dirname);
	}
	else {
		strcat(extractzipfile, job_store.name);
	}

	strcat(extractzipfile, "/");
	strcat(extractzipfile, job_store.name);
	strcat(extractzipfile, ".zip");

	while (extract_attempt < 3) {
		if (setting_use_sd) {
			if (unzipArchive(extractzipfile, "sd:/")) {

				if (strcmp(job_store.name,"ftpii") == 0 && strcmp(job_store.user_dirname,"ftpii") != 0) {
					char renamed[100] = "sd:/apps/";
					strcat(renamed,job_store.user_dirname);
					strcat(renamed,"/boot.dol");

					char renamed2[100] = "sd:/apps/";
					strcat(renamed2,job_store.user_dirname);
					strcat(renamed2,"/meta.xml");

					remove_file(renamed);
					rename("sd:/apps/ftpii/boot.dol",renamed);
					remove_file(renamed2);
					rename("sd:/apps/ftpii/meta.xml",renamed2);
					remove_dir("sd:/apps/ftpii/");
				}

				break;
			}
			else {
				extract_attempt++;
			}
		}
		else {
			if (unzipArchive(extractzipfile, "usb:/")) {

				if (strcmp(job_store.name,"ftpii") == 0 && strcmp(job_store.user_dirname,"ftpii") != 0) {
					char renamed[100] = "usb:/apps/";
					strcat(renamed,job_store.user_dirname);
					strcat(renamed,"/boot.dol");

					char renamed2[100] = "usb:/apps/";
					strcat(renamed2,job_store.user_dirname);
					strcat(renamed2,"/meta.xml");

					remove_file(renamed);
					rename("usb:/apps/ftpii/boot.dol",renamed);
					remove_file(renamed2);
					rename("usb:/apps/ftpii/meta.xml",renamed2);
					remove_dir("usb:/apps/ftpii/");
				}

				break;
			}
			else {
				extract_attempt++;
			}
		}
	}

	if (cancel_extract) {
		extract_status = false;
		extract_in_progress = -1;
		error_number = 4;
	}

	if (extract_attempt >= 3) {
		extract_status = false;
		extract_in_progress = -1;
		error_number = 4;
	}

	remove_file(extractzipfile);
	usleep(200000);

	// We need to recheck the boot.dol/elf file size as the file has changed
	char boot_path[100] = "sd:/apps/";
	char theme_path[100];
	if (!setting_use_sd) {
		strcpy(boot_path,"usb:/apps/");
	}
	if (strcmp(job_store.name,"ftpii") == 0) {
		strcat(boot_path, job_store.user_dirname);
	}
	else {
		strcat(boot_path, job_store.name);
	}
	strcpy(theme_path, boot_path);
	strcat(theme_path, "/theme.zip");
	strcat(boot_path, "/boot.");
	strcat(boot_path, job_store.boot_ext);

	FILE *f = fopen(boot_path, "rb");

	// Problems opening the file?
	if (f == NULL) {

		FILE *ftheme = fopen(theme_path, "rb");

		if (ftheme == NULL) {
			if (extract_status) {
				extract_status = false;
				error_number = 5;
			}
		}
		else {
			// Open file and get the file size
			fseek (ftheme , 0, SEEK_END);
			job_store.local_app_size = ftell (ftheme);
			rewind (ftheme);
			fclose(ftheme);
		}
	}
	else {
		// Open file and get the file size
		fseek (f , 0, SEEK_END);
		job_store.local_app_size = ftell (f);
		rewind (f);
		fclose(f);

		// Rename it to what they had before (hb sorter)
		if (job_store.boot_bak) {
			char boot_path_bak[100];
			strcpy(boot_path_bak, boot_path);
			strcat(boot_path_bak, ".bak");
			rename(boot_path, boot_path_bak);
		}

	}

	// Failed or success?
	if (!extract_status) {
		extract_in_progress = -1;
	}
	else {
		job_store.in_download_queue = false;
		extract_in_progress = false;
		sd_card_update = true;

		total_list[job_store.original_pos] = job_store;

		if (updating >= 0 && updating < int_array_length(current_items)) {
			new_updating++;
		}
		if (update_xml == 1) {
			update_xml = 2;
		}
	}

	if (ACTIVITIES_current() != ACTIVITY_APP) {
		download_in_progress = false;
		if (extract_in_progress != -1) {
			extract_in_progress = false;
		}
		delete_in_progress = false;
		cancel_download = false;
		cancel_extract = false;
	}

	total_list[job_store.original_pos] = job_store;

	return 0;
}

u8 initialise_download() {
	s32 result = LWP_CreateThread(&download_thread, run_download_thread, NULL, NULL, 0, 80);
	return result;
}

static void *run_delete_thread(void *arg) {

	if (error_number == 0) {
		clear_store_list();
		job_store = total_list[current_items[selected_app]];
	}

	bool delete_status = true;
	bool ok_to_del = true;
	char no_del_list[50][300];
	int no_del_count = 0;

	// Directories to delete all files from
	if (job_store.folders != NULL && delete_status && !cancel_delete) {
		char folders[1000];
		strcpy(folders, job_store.folders_no_del);
		char *split_tok;
		split_tok = strtok (folders,";");

		while (split_tok != NULL) {
			strcpy(no_del_list[no_del_count], split_tok);
			no_del_count++;
			split_tok = strtok (NULL, ";");
		}

		char folders1[1000];
		strcpy(folders1, job_store.folders);

		char *split_tok1;
		split_tok1 = strtok (folders1,";");

		while (split_tok1 != NULL) {
			ok_to_del = true;

			int d;
			for (d = 0; d < no_del_count; d++) {
				if (strcmp(no_del_list[d], split_tok1) == 0) {
					ok_to_del = false;
				}
			}

			if (ok_to_del) {
				char temp_del[200] = "sd:";
				if (!setting_use_sd) {
					strcpy(temp_del,"usb:");
				}
				strcat(temp_del, split_tok1);

				delete_dir_files(temp_del);
			}

			split_tok1 = strtok (NULL, ";");

		}
	}

	// Delete the icon.png, meta.xml and boot.dol/elf
	if (delete_status) {
		char del_file[150] = "sd:/apps/";
		if (!setting_use_sd) {
			strcpy(del_file,"usb:/apps/");
		}

		if (strcmp(job_store.name,"ftpii") == 0) {
			strcat(del_file, job_store.user_dirname);
		}
		else {
			strcat(del_file, job_store.name);
		}

		strcat(del_file, "/icon.png");

		if (remove_file(del_file) != 1) {
			delete_status = false;
			error_number = 6;
		}
	}
	if (delete_status) {
		char del_file[150] = "sd:/apps/";
		if (!setting_use_sd) {
			strcpy(del_file,"usb:/apps/");
		}

		if (strcmp(job_store.name,"ftpii") == 0) {
			strcat(del_file, job_store.user_dirname);
		}
		else {
			strcat(del_file, job_store.name);
		}

		strcat(del_file, "/meta.xml");

		if (remove_file(del_file) != 1) {
			delete_status = false;
			error_number = 6;
		}
	}
	if (delete_status) {
		char del_file[150] = "sd:/apps/";
		if (!setting_use_sd) {
			strcpy(del_file,"usb:/apps/");
		}
		if (strcmp(job_store.name,"ftpii") == 0) {
			strcat(del_file, job_store.user_dirname);
		}
		else {
			strcat(del_file, job_store.name);
		}

		strcat(del_file, "/boot.");
		strcat(del_file, job_store.boot_ext);

		// hb sorter
		if (job_store.boot_bak) {
			strcat(del_file, ".bak");
		}

		if (remove_file(del_file) != 1) {
			delete_status = false;
			error_number = 6;
		}

		// Theme.zip
		char del_file1[150] = "sd:/apps/";
		if (!setting_use_sd) {
			strcpy(del_file1,"usb:/apps/");
		}
		if (strcmp(job_store.name,"ftpii") == 0) {
			strcat(del_file1, job_store.user_dirname);
		}
		else {
			strcat(del_file1, job_store.name);
		}

		strcat(del_file1, "/theme.zip");

		if (remove_file(del_file) != 1) {
			delete_status = false;
			error_number = 6;
		}
	}

	usleep(100000);

	// Delete all created folders
	char directory[150][300];
	int remove_count = 0;

	if (job_store.folders != NULL && delete_status && !cancel_delete) {
		char folders[1000];
		strcpy(folders, job_store.folders);

		char *split_tok;
		split_tok = strtok (folders,";");

		while (split_tok != NULL && delete_status) {

			strcpy(directory[remove_count],split_tok);
			remove_count++;
			split_tok = strtok (NULL, ";");
		}

		int x = 0;

		// Delete each folder, starting from the last to the first
		for (x = remove_count-1; x >= 0; x--) {

			ok_to_del = true;

			int d;
			for (d = 0; d < no_del_count; d++) {
				if (strcmp(no_del_list[d], directory[x]) == 0) {
					ok_to_del = false;
				}
			}

			if (delete_status && ok_to_del) {
				char temp_del[200] = "sd:";
				if (!setting_use_sd) {
					strcpy(temp_del,"usb:");
				}
				strcat(temp_del, directory[x]);
			}
		}
	}

	// Delete all files from main folder
	if (delete_status) {
		char main_folder[300] = "/apps/";
		strcat(main_folder, job_store.name);

		ok_to_del = true;

		int d;
		for (d = 0; d <= no_del_count; d++) {
			if (strcmp(no_del_list[d], main_folder) == 0) {
				ok_to_del = false;
			}
		}

		if (ok_to_del) {
			char app_dir[100] = "sd:/apps/";
			if (!setting_use_sd) {
				strcpy(app_dir,"usb:/apps/");
			}
			if (strcmp(job_store.name,"ftpii") == 0) {
				strcat(app_dir, job_store.user_dirname);
			}
			else {
				strcat(app_dir, job_store.name);
			}

			delete_dir_files(app_dir);
		}
	}

	// Standard directory to delete
	if (delete_status) {
		char app_dir[100] = "sd:/apps/";
		if (!setting_use_sd) {
			strcpy(app_dir,"usb:/apps/");
		}
		if (strcmp(job_store.name,"ftpii") == 0) {
			strcat(app_dir, job_store.user_dirname);
		}
		else {
			strcat(app_dir, job_store.name);
		}
	}

	// Failed or success?
	if (!delete_status) {
		delete_in_progress = -1;
		cancel_delete = false;
	}
	else {
		job_store.local_app_size = 0;
		job_store.in_download_queue = false;
		delete_in_progress = false;
		sd_card_update = true;
		cancel_delete = false;

		if (updating >= 0 && updating < int_array_length(current_items)) {
			new_updating++;
		}
	}

	total_list[job_store.original_pos] = job_store;

	return 0;
}

u8 initialise_delete() {
	s32 result = LWP_CreateThread(&delete_thread, run_delete_thread, NULL, NULL, 0, 80);
	return result;
}

static void *run_request_thread(void *arg) {
	if (setting_online) {
		if (setting_repo == 0) {
			if (setting_use_sd) {
				if (create_and_request_file("sd:/apps/", "homebrew_browser_lite", "/listv036.txt") == 1) {
					UI_bootScreen("Homebrew list received");
					list_received = true;
				}
			}
			else {
				if (create_and_request_file("usb:/apps/", "homebrew_browser_lite", "/listv036.txt") == 1) {
					UI_bootScreen("Homebrew list received");
					list_received = true;
				}
			}
		}
		else {
			if (setting_use_sd) {
				if (request_list_file("sd:/apps/homebrew_browser_lite/external_repo_list.txt", repo_list[setting_repo].list_file) == 1) {
					UI_bootScreen("Homebrew list received");
					list_received = true;
				}
			}
			else {
				if (request_list_file("usb:/apps/homebrew_browser_lite/external_repo_list.txt", repo_list[setting_repo].list_file) == 1) {
					UI_bootScreen("Homebrew list received");
					list_received = true;
				}
			}
		}

	}
	else if (setting_repo == 0) {
		UI_bootScreen("Using local Homebrew list");
		list_received = true;
	}
	else {
		UI_bootScreen("Cannot use local Homebrew list");
	}

	return 0;
}

u8 initialise_request() {
	s32 result = LWP_CreateThread(&request_thread, run_request_thread, NULL, NULL, 0, 80);
	return result;
}


// Suspend reset thread
void suspend_reset_thread() {
	LWP_SuspendThread(reset_thread);
}


// Return the array length by counting the characters in the name
int array_length(struct homebrew_struct array[]) {
	int x = 0;
	while (strlen(array[x].name) >= 2) {
		x++;
	}
	return x;
}

int int_array_length(int array[]) {
	int x = 0;
	while (array[x] != HOMEBREW_STRUCT_END) {
		x++;
	}
	return x;
}

// Add text to log
void add_to_log(char* text, ...) {

	va_list args;
	va_start (args, text);

	char log_file[60];
	strcpy(log_file, rootdir);
	strcat(log_file, "/apps/homebrew_browser_lite/log.txt");
	FILE *flog = fopen(log_file, "a");
	vfprintf (flog, text, args);
	fputs ("\r\n", flog);
	fclose(flog);

	va_end (args);
}

void update_settings() {
	mxml_node_t *xml;
	mxml_node_t *data;

	xml = mxmlNewXML("1.0");

	data = mxmlNewElement(xml, "settings");

	char set2[2];
	sprintf(set2, "%i", setting_sd_card);
	mxmlElementSetAttr(data, "setting_sd_card", set2);
	char set3[2];
	sprintf(set3, "%i", setting_hide_installed);
	mxmlElementSetAttr(data, "setting_hide_installed", set3);
	char set6[2];
	sprintf(set6, "%i", setting_online);
	mxmlElementSetAttr(data, "setting_online", set6);
	char set7[2];
	sprintf(set7, "%i", setting_rumble);
	mxmlElementSetAttr(data, "setting_rumble", set7);
	char set8[2];
	sprintf(set8, "%i", setting_update_icon);
	mxmlElementSetAttr(data, "setting_update_icon", set8);
	char set9[2];
	sprintf(set9, "%i", setting_tool_tip);
	mxmlElementSetAttr(data, "setting_tool_tip", set9);
	mxmlElementSetAttr(data, "setting_last_boot", setting_last_boot);
	char set13[2];
	sprintf(set13, "%i", setting_use_sd);
	mxmlElementSetAttr(data, "setting_use_sd", set13);
	char set14[3];
	sprintf(set14, "%i", setting_repo);
	mxmlElementSetAttr(data, "setting_repo", set14);
	char set15[2];
	sprintf(set15, "%i", setting_sort);
	mxmlElementSetAttr(data, "setting_sort", set15);
	char set16[2];
	sprintf(set16, "%i", setting_category);
	mxmlElementSetAttr(data, "setting_category", set16);
	char set17[2];
	sprintf(set17, "%i", setting_disusb);
	mxmlElementSetAttr(data, "setting_disusb", set17);
	char set19[2];
	sprintf(set19, "%i", setting_wiiside);
	mxmlElementSetAttr(data, "setting_wiiside", set19);

	FILE *fp = fopen("sd:/apps/homebrew_browser_lite/settings.xml", "wb");
	FILE *fp1 = fopen("usb:/apps/homebrew_browser_lite/settings.xml", "wb");

	if (fp != NULL) {
		mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
		fclose(fp);
	}

	if (fp1 != NULL) {
		mxmlSaveFile(xml, fp1, MXML_NO_CALLBACK);
		fclose(fp1);
	}

	mxmlDelete(data);
	mxmlDelete(xml);
}

void load_mount_settings() {
	mxml_node_t *tree;
	mxml_node_t *data;

	FILE *fp = fopen("sd:/apps/homebrew_browser_lite/settings.xml", "rb");

	if (fp != NULL) {
		fseek (fp , 0, SEEK_END);
		long settings_size = ftell (fp);
		rewind (fp);

		if (settings_size > 0) {

			tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
			fclose(fp);

			data = mxmlFindElement(tree, tree, "settings", NULL, NULL, MXML_DESCEND);

			if (mxmlElementGetAttr(data,"setting_disusb")) {
				setting_disusb = atoi(mxmlElementGetAttr(data,"setting_disusb"));
			}

			mxmlDelete(data);
			mxmlDelete(tree);

		}
	}
}

void load_settings() {
	mxml_node_t *tree;
	mxml_node_t *data;

	FILE *fp = NULL;
	int loaded_from = true;

	if (sd_mounted) {
		fp = fopen("sd:/apps/homebrew_browser_lite/settings.xml", "rb");
		if (fp == NULL) {
			fclose(fp);
			fp = fopen("usb:/apps/homebrew_browser_lite/settings.xml", "rb");
			loaded_from = false;
		}
	}
	else {
		fp = fopen("usb:/apps/homebrew_browser_lite/settings.xml", "rb");
		loaded_from = false;
	}

	if (fp != NULL) {
		fseek (fp , 0, SEEK_END);
		long settings_size = ftell (fp);
		rewind (fp);

		if (settings_size > 0) {

			tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
			fclose(fp);

			data = mxmlFindElement(tree, tree, "settings", NULL, NULL, MXML_DESCEND);

			if (mxmlElementGetAttr(data,"setting_sd_card")) {
				setting_sd_card = atoi(mxmlElementGetAttr(data,"setting_sd_card"));
			}
			if (mxmlElementGetAttr(data,"setting_hide_installed")) {
				setting_hide_installed = atoi(mxmlElementGetAttr(data,"setting_hide_installed"));
			}
			if (mxmlElementGetAttr(data,"setting_online") && setting_online) {
				setting_online = atoi(mxmlElementGetAttr(data,"setting_online"));
			}
			if (mxmlElementGetAttr(data,"setting_rumble")) {
				setting_rumble = atoi(mxmlElementGetAttr(data,"setting_rumble"));
			}
			if (mxmlElementGetAttr(data,"setting_update_icon")) {
				setting_update_icon = atoi(mxmlElementGetAttr(data,"setting_update_icon"));
			}
			if (mxmlElementGetAttr(data,"setting_tool_tip")) {
				setting_tool_tip = atoi(mxmlElementGetAttr(data,"setting_tool_tip"));
			}
			if (mxmlElementGetAttr(data,"setting_last_boot")) {
				if (atoi(mxmlElementGetAttr(data,"setting_last_boot")) > 0) {
					strcpy(setting_last_boot, mxmlElementGetAttr(data,"setting_last_boot"));
				}
			}
			if (mxmlElementGetAttr(data,"setting_use_sd")) {
				setting_use_sd = atoi(mxmlElementGetAttr(data,"setting_use_sd"));
			}
			if (mxmlElementGetAttr(data,"setting_repo")) {
				if (atoi(mxmlElementGetAttr(data,"setting_repo")) >= 0) {
					setting_repo = atoi(mxmlElementGetAttr(data,"setting_repo"));
				}
			}
			if (mxmlElementGetAttr(data,"setting_sort")) {
				if (atoi(mxmlElementGetAttr(data,"setting_sort")) >= 0) {
					setting_sort = atoi(mxmlElementGetAttr(data,"setting_sort"));
				}
			}
			if (mxmlElementGetAttr(data,"setting_category")) {
				if (atoi(mxmlElementGetAttr(data,"setting_category")) >= 0) {
					setting_category = atoi(mxmlElementGetAttr(data,"setting_category"));
				}
			}
			if (mxmlElementGetAttr(data,"setting_disusb")) {
				setting_disusb = atoi(mxmlElementGetAttr(data,"setting_disusb"));
			}
			if (mxmlElementGetAttr(data,"setting_wiiside")) {
				setting_wiiside = atoi(mxmlElementGetAttr(data,"setting_wiiside"));
			}

			mxmlDelete(data);
			mxmlDelete(tree);

			if (loaded_from) {
				UI_bootScreen("Settings loaded from SD card");
			}
			else {
				UI_bootScreen("Settings loaded from USB device");
			}

			// Double check that setting SD is correct
			if (setting_use_sd && !sd_mounted) {
				setting_use_sd = false;
				UI_bootScreenTwo("Settings say to load from SD, no SD found", "Using USB instead");
			}
			else if (!setting_use_sd && !usb_mounted) {
				setting_use_sd = true;
				UI_bootScreenTwo("Settings say to load from USB, no USB found", "Using SD instead");
			}
		}
		else {
			if (loaded_from) {
				remove_file("sd:/apps/homebrew_browser_lite/settings.xml");
			}
			else {
				remove_file("sd:/apps/homebrew_browser_lite/settings.xml");
			}
		}
	}
	fclose(fp);

	// Setting repo revert to codemii
	if (setting_repo_revert) {
		setting_repo = 0;
	}

	// What device to use?
	if (setting_use_sd) {
		strcpy(rootdir, "sd:/");
	}
	else {
		strcpy(rootdir, "usb:/");
	}
}

// Save the meta.xml "name" element
void save_xml_name() {

	char savexml[150] = "sd:/apps/";
	if (!setting_use_sd) {
		strcpy(savexml,"usb:/apps/");
	}
	if (strcmp(total_list[current_items[selected_app]].name,"ftpii") == 0) {
		strcat(savexml, total_list[current_items[selected_app]].user_dirname);
	}
	else {
		strcat(savexml, total_list[current_items[selected_app]].name);
	}
	strcat(savexml, "/meta.xml");

	FILE *fx = fopen(savexml, "rb");
	if (fx != NULL) {
		mxml_node_t *tree;
		tree = mxmlLoadFile(NULL, fx, MXML_OPAQUE_CALLBACK);
		fclose(fx);

		mxml_node_t *thename = mxmlFindElement(tree, tree, "name", NULL, NULL, MXML_DESCEND);
		if (thename == NULL) { strcpy(temp_name, "0"); }
		else { strcpy(temp_name, thename->child->value.opaque); }

		mxmlDelete(tree);
	}

	fclose(fx);
}

void copy_xml_name() {

	if (strcmp(temp_name,"0") != 0) {

		char savexml[150] = "sd:/apps/";
		if (!setting_use_sd) {
			strcpy(savexml,"usb:/apps/");
		}
		if (strcmp(total_list[current_items[selected_app]].name,"ftpii") == 0) {
			strcat(savexml, total_list[current_items[selected_app]].user_dirname);
		}
		else {
			strcat(savexml, total_list[current_items[selected_app]].name);
		}
		strcat(savexml, "/meta.xml");

		FILE *fx = fopen(savexml, "rb");
		if (fx != NULL) {
			mxml_node_t *tree1;
			tree1 = mxmlLoadFile(NULL, fx, MXML_OPAQUE_CALLBACK);
			fclose(fx);

			mxml_node_t *thename1 = mxmlFindElement(tree1, tree1, "name", NULL, NULL, MXML_DESCEND);
			mxmlSetOpaque(thename1->child, temp_name);

			fx = fopen(savexml, "wb");
			if (fx != NULL) {
				mxmlSaveFile(tree1, fx, 0);
			}
			fclose(fx);

			mxmlDelete(tree1);
		}
	}
}

static int compareNamesTrue(const void *p1, const void *p2) {
	const int *elem1 = p1;
	const int *elem2 = p2;
	if ((*elem1) == HOMEBREW_STRUCT_END) return 1;
	else if ((*elem2) == HOMEBREW_STRUCT_END) return -1;
	else return strcasecmp(total_list[(*elem1)].name, total_list[(*elem2)].name);
}

static int compareNamesFalse(const void *p1, const void *p2) {
	const int *elem1 = p1;
	const int *elem2 = p2;
	if ((*elem1) == HOMEBREW_STRUCT_END) return 1;
	else if ((*elem2) == HOMEBREW_STRUCT_END) return -1;
	else return strcasecmp(total_list[(*elem2)].name, total_list[(*elem1)].name);
}

void sort_by_name (bool min_to_max) {
	if (min_to_max) qsort(current_items, HOMEBREW_STRUCT_SIZE, sizeof(int), compareNamesTrue);
	else qsort(current_items, HOMEBREW_STRUCT_SIZE, sizeof(int), compareNamesFalse);
}

static int compareDatesTrue(const void *p1, const void *p2) {
	const int *elem1 = p1;
	const int *elem2 = p2;
	if ((*elem1) == HOMEBREW_STRUCT_END) return 1;
	else if ((*elem2) == HOMEBREW_STRUCT_END) return -1;
	else return (total_list[(*elem1)].app_time - total_list[(*elem2)].app_time);
}

static int compareDatesFalse(const void *p1, const void *p2) {
	const int *elem1 = p1;
	const int *elem2 = p2;
	if ((*elem1) == HOMEBREW_STRUCT_END) return 1;
	else if ((*elem2) == HOMEBREW_STRUCT_END) return -1;
	else return (total_list[(*elem2)].app_time - total_list[(*elem1)].app_time);
}

void sort_by_date (bool min_to_max) {
	if (min_to_max) qsort(current_items, HOMEBREW_STRUCT_SIZE, sizeof(int), compareDatesTrue);
	else qsort(current_items, HOMEBREW_STRUCT_SIZE, sizeof(int), compareDatesFalse);
}

void hide_apps_installed() {
	clear_temp_list();

	int x;
	for (x = 0; x < int_array_length(current_items); x++) {
		temp_list[x] = current_items[x];
	}

	clear_list();

	int i;
	int j = 0;
	for (i = 0; i < int_array_length(temp_list); i++) {
		if (total_list[temp_list[i]].local_app_size == 0) {
			current_items[j] = temp_list[i];
			j++;
		}
	}
	if (j == 0) {
		current_items[i] = HOMEBREW_STRUCT_END;
	}
}

void die(char *msg) {
	UI_bootScreen(msg);
	sleep(5);
	fatUnmount("sd:");
	fatUnmount("usb:");
	exitApp(1);
}

void initialise() {
	VIDEO_Init();
	WPAD_Init();
	PAD_Init();
	WPAD_SetVRes(0, 640, 480);
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
	vmode = VIDEO_GetPreferredMode(NULL);
	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	console_init(xfb[0],20,20,vmode->fbWidth,vmode->xfbHeight,vmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(vmode);
	VIDEO_SetNextFramebuffer(xfb[0]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	ASND_Init();
	xfb_height = vmode->xfbHeight;
}


static bool can_open_root_fs() {

	DIR *root;
	root = opendir(rootdir);
	if (root) {
		closedir(root);
		return true;
	}
	return false;
}

void initialise_fat() {

	bool fat_init = false;

	// At least one FAT initialisation has to be completed
	UI_bootScreen("Attempting to mount SD card");
	if (initialise_device(METHOD_SD)) {
		strcpy(rootdir, "sd:/");
		if (test_fat()) {
			fat_init = true;
			sd_mounted = true;
			UI_bootScreen("SD card mounted");
			load_mount_settings();
		}
		else {
			fatUnmount("sd:");
			sleep(1);
		}
	}
	if (!setting_disusb) {
		UI_bootScreen("Attempting to mount USB device");
		if (initialise_device(METHOD_USB)) {
			strcpy(rootdir, "usb:/");
			if (test_fat()) {
				fat_init = true;
				usb_mounted = true;
				UI_bootScreen("USB device mounted");
			}
			else {
				fatUnmount("usb:");
				sleep(1);
			}
		}
	}

	if (!fat_init)
	{
		UI_bootScreen("Could not mount SD card or USB device");
		sleep(5);
		exitApp(0);
	}

}

bool initialise_device(int method) {

	bool mounted = false;
	char name[10];
	const DISC_INTERFACE* disc = NULL;

	switch(method)
	{
			case METHOD_SD:
					sprintf(name, "sd");
					disc = sd;
					break;
			case METHOD_USB:
					sprintf(name, "usb");
					disc = usb;
					break;
	}

	if (disc->startup() && fatMountSimple(name, disc)) {
		mounted = true;
	}
	else {
		UI_bootScreen("Unable to mount device");
	}

	return mounted;
}

bool test_fat() {

	// Try to open root filesystem - if we don't check here, mkdir crashes later
	if (!can_open_root_fs()) {
		printf("Unable to open root filesystem of %s\n", rootdir);
		return false;
	}

	// Change dir
	if (chdir(rootdir)) {
		printf("Could not change to root directory to %s\n", rootdir);
		return false;
	}

	// Create directories
	char dir_apps[50];
	char dir_hbb[150];
	char dir_hbbtemp[150];

	strcpy(dir_apps, rootdir);
	strcat(dir_apps, "apps");
	strcpy(dir_hbb, rootdir);
	strcat(dir_hbb, "apps/homebrew_browser_lite");
	strcpy(dir_hbbtemp, rootdir);
	strcat(dir_hbbtemp, "apps/homebrew_browser_lite/temp");

	if (!opendir(dir_apps)) {
		mkdir(dir_apps, 0777);
		if (!opendir(dir_apps)) {
			printf("Could not create %s directory.\n", dir_apps);
		}
	}


	if (!opendir(dir_hbb)) {
		mkdir(dir_hbb, 0777);
		if (!opendir(dir_hbb)) {
			printf("Could not create %s directory.\n", dir_hbb);
		}
	}

	if (!opendir(dir_hbbtemp)) {
		mkdir(dir_hbbtemp, 0777);
		if (!opendir(dir_hbbtemp)) {
			printf("Could not create %s directory.\n", dir_hbbtemp);
		}
	}

	return true;
}

void initialise_network() {
	UI_bootScreen("Initializing Network");

	// Tantric code from Snes9x-gx
	s32 res=-1;
	int retry;
	int wait;

	retry = 5;

	while (retry>0)
	{
		int i;
		net_deinit();
		for(i=0; i < 400; i++) // 10 seconds to try to reset
		{
			res = net_get_status();
			if(res != -EBUSY) // trying to init net so we can't kill the net
			{
				usleep(2000);
				net_wc24cleanup(); //kill the net
				usleep(20000);
				break;
			}
			usleep(20000);
		}

		usleep(2000);
		res = net_init_async(NULL, NULL);

		if(res != 0)
		{
			sleep(1);
			retry--;
			continue;
		}

		res = net_get_status();
		wait = 400; // only wait 8 sec
		while (res == -EBUSY && wait > 0)
		{
			usleep(20000);
			res = net_get_status();
			wait--;
		}

		if(res==0) break;
		retry--;
		usleep(2000);
	}
	if (res == 0)
	{
		struct in_addr hostip;
		hostip.s_addr = net_gethostip();
		if (hostip.s_addr) {
			UI_bootScreen("Network initialized");
		} else {
			UI_bootScreen("Could not connect to network");
			sleep(5);
			exitApp(0);
		}
	}

}

bool check_wifi() {
	s32 result = -1;
	int times = 0;

	while (result < 0 && times < 3) {
		while ((result = net_init()) == -EAGAIN) {
		}
		if (result < 0) printf("Unable to initialise network, retrying...\n");
		times++;
	}
	if (result >= 0 && times < 3) {
		u32 ip = 0;
		do {
			ip = net_gethostip();
			if (!ip) printf("Unable to initialise network, retrying...\n");
			times++;
		} while (!ip && times < 3);
		if (ip) printf("Network initialised");
	}
	if (times >= 3) {
		return 0;
	}
	return 1;
}

void initialise_codemii() {
	UI_bootScreen("Requesting IP address of " MAIN_DOMAIN);
	initializedns();
	IP_ADDRESS = getipbynamecached(MAIN_DOMAIN);

	if (IP_ADDRESS == 0) {
		UI_bootScreen("Failed, using stored IP address");
		hostname_ok = false;
	}
	else {
		UI_bootScreen("IP address successfully retrieved");
	}
}

void initialise_codemii_backup() {
	UI_bootScreen("Requesting IP address of " FALLBACK_DOMAIN);
	hostname_ok = true;
	IP_ADDRESS = getipbynamecached(FALLBACK_DOMAIN);

	if (IP_ADDRESS == 0) {
		UI_bootScreen("Failed, using stored IP address");
		hostname_ok = false;
	}
	else {
		UI_bootScreen("IP address successfully retrieved");
	}
}

typedef s32 (*transferrer_type)(s32 s, void *mem, s32 len);
inline static s32 transfer_exact(s32 s, char *buf, s32 length, transferrer_type transferrer) {
	s32 bytes_transferred = 0;
	s32 remaining = length;
	while (remaining) {
		if ((bytes_transferred = transferrer(s, buf, remaining > NET_BUFFER_SIZE ? NET_BUFFER_SIZE : remaining)) > 0) {
			remaining -= bytes_transferred;
			buf += bytes_transferred;
		} else if (bytes_transferred < 0) {
			return bytes_transferred;
		} else {
			return -ENODATA;
		}
	}
	return 0;
}

inline s32 write_exact(s32 s, char *buf, s32 length) {
	return transfer_exact(s, buf, length, (transferrer_type)net_write);
}


int load_file_to_memory(const char *filename, unsigned char **result) {
	int size = 0;
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		*result = NULL;
		return -1; // -1 means file opening fail
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (unsigned char *)malloc(size+1);
	if (size != fread(*result, sizeof(char), size, f)) {
		free(*result);
		return -1; // -2 means file reading fail
	}
	fclose(f);
	(*result)[size] = 0;
	return 1;
}

// Clear list
void clear_list() {
	int c;
	for (c = 0; c < HOMEBREW_STRUCT_SIZE; c++) {
		current_items[c] = HOMEBREW_STRUCT_END;
	}
}

// Clear list
void clear_temp_list() {
	int c;
	for (c = 0; c < HOMEBREW_STRUCT_SIZE; c++) {
		temp_list[c] = HOMEBREW_STRUCT_END;
	}
}

// Clear list
void clear_store_list() {
	job_store.name[0] = 0;
	job_store.app_size = 0;
	job_store.app_time = 0;
	job_store.img_size = 0;
	job_store.local_app_size = 0;
	job_store.total_app_size = 0;
	job_store.in_download_queue = 0;
	job_store.user_dirname[0] = 0;
	job_store.folders[0] = 0;
	job_store.boot_ext[0] = 0;
	job_store.boot_bak = 0;
	job_store.no_manage = 0;

	job_store.about_loaded = 0;
	job_store.app_name[0] = 0;
	job_store.app_short_description[0] = 0;
	job_store.app_description[0] = 0;
	job_store.app_author[0] = 0;
	job_store.app_version[0] = 0;
	job_store.app_total_size = 0;
	job_store.app_controllers[0] = 0;

	job_store.file_found = 0;
	job_store.content = NULL;

	job_store.original_pos = HOMEBREW_STRUCT_END;
}


// Removes a file
int remove_file(char* path) {
	FILE *f = fopen(path, "rb");

	// File not found
	if (f == NULL) {
		return 1;
	}
	else {
		fclose(f);
		unlink(path);

		// Check file was removed
		f = fopen(path, "rb");

		if (f == NULL) {
			return 1;
		}
		fclose(f);
	}

	return -1;
}

// Removes a directory
int remove_dir(char* path) {
	if (opendir(path)) {
		unlink(path);
		if (opendir(path)) {
			return -1;
		}
	}

	return 1;
}

// Delete all files in a directory
int delete_dir_files(char* path) {

	dir = opendir(path);

	if (dir != NULL) {
		char temp_path[MAXPATHLEN];
		while ((dent=readdir(dir)) != NULL) {
			strcpy(temp_path, path);
			strcat(temp_path, "/");
			strcat(temp_path, dent->d_name);
			stat(temp_path, &st);

			if(!(S_ISDIR(st.st_mode))) {
				remove_file(temp_path);
			}
		}
	}
	closedir(dir);

	return 1;
}

// Creates a directory
int create_dir(char* path) {
	if (!opendir(path)) {
		mkdir(path, 0777);
		if (!opendir(path)) {
			return -1;
		}
	}

	return 1;
}

// Unzip Archive
bool unzipArchive(char * zipfilepath, char * unzipfolderpath) {
	unzFile uf = unzOpen(zipfilepath);

	if (uf==NULL) {
		printf("Cannot open %s, aborting\n",zipfilepath);
		return false;
	}

	if (chdir(unzipfolderpath)) { // can't access dir
		makedir(unzipfolderpath); // attempt to make dir
		if (chdir(unzipfolderpath)) { // still can't access dir
			printf("Error changing into %s, aborting\n", unzipfolderpath);
			return false;
		}
	}

	do_extract(uf,0,1,0);

	unzCloseCurrentFile(uf);
	return true;
}


void download_queue_size() {
	updating_current_size = 0;
	updating_total_size = 0;

	int x;
	for (x = 0; x < int_array_length(current_items); x++) {
		if (current_items[x] != HOMEBREW_STRUCT_END && total_list[current_items[x]].in_download_queue != 2) {
			updating_total_size += total_list[current_items[x]].total_app_size;
		}
	}

	updating_part_size = updating_total_size / 100;
}


void check_temp_files() {
	char prefix[16] = "sd:/apps/";
	char path[64] = "sd:/apps/homebrew_browser_lite/temp/";
	char file[64] = "sd:/apps/homebrew_browser_lite/temp_files.zip";
	if (!setting_use_sd) {
		strcpy(prefix, "usb:/apps/");
		strcpy(path, "usb:/apps/homebrew_browser_lite/temp/");
		strcpy(file, "usb:/apps/homebrew_browser_lite/temp_files.zip");
	}
	int x = 0;

	dir = opendir(path);

	if (dir != NULL) {
		char temp_path[MAXPATHLEN];
		while ((dent=readdir(dir)) != NULL) {
			strcpy(temp_path, path);
			strcat(temp_path, "/");
			strcat(temp_path, dent->d_name);
			stat(temp_path, &st);

			if(!(S_ISDIR(st.st_mode))) {
				x++;
				if (x > 200) {
					break;
				}
			}
		}
	}
	closedir(dir);

	if (x < total_list_count) {
		UI_bootScreenTwo("Downloading latest previews", "Hold down B to skip");

		hbb_updating = true;
		remote_hb_size = 1874386;

		if (create_and_request_file(prefix, "homebrew_browser_lite", "/temp_files.zip") != 1) {
			UI_bootScreen("Failed downloading previews");
		}
		else {
			UI_bootScreen("Extracting previews");
			if (unzipArchive(file, path)) {
				if (!cancel_extract) {
					UI_bootScreen("Downloaded and extracted previews successfully");
				}
			}
		}
		remove_file(file);
		hbb_updating = false;

		if (cancel_download || cancel_extract) {
			UI_bootScreen("Cancelled downloading and extracting of previews");
			cancel_download = false;
			cancel_extract = false;
		}
	}
}

void repo_check() {
	// Setting in settings which will just be a number, 0 for HBB, 1 for another one, etc.
	// Always grab the listing of other repos

	UI_bootScreen("Requesting repositories list");

	s32 main_server = server_connect(1);

	char http_request[1000];
	strcpy(http_request,"GET /hbb/repo_list.txt");

	if (!codemii_backup) {
		strcat(http_request," HTTP/1.0\r\nHost: " MAIN_DOMAIN "\r\nCache-Control: no-cache\r\n\r\n");
	} else {
		strcat(http_request," HTTP/1.0\r\nHost: " FALLBACK_DOMAIN "\r\nCache-Control: no-cache\r\n\r\n");
	}

	write_http_reply(main_server, http_request);

	bool http_data = false;
	char buf[BUFFER_SIZE];
	s32 offset = 0;
	s32 bytes_read;
	int count = 0;
	while (offset < (BUFFER_SIZE - 1)) {
		char *offset_buf = buf + offset;
		if ((bytes_read = net_read(main_server, offset_buf, BUFFER_SIZE - 1 - offset)) < 0) {
			UI_bootScreen(get_error_msg(bytes_read));
			printf("%s\nError code %i in repo_check\n\n", get_error_msg(bytes_read), bytes_read);
			net_close(main_server);
			sleep(1);
		} else if (bytes_read == 0) {
			break; // EOF from client
		}
		offset += bytes_read;
		buf[offset] = '\0';

		char *next;
		char *end;
		for (next = buf; (end = strstr(next, CRLF)); next = end + CRLF_LENGTH) {
			*end = '\0';

			if (*next) {
				char *cmd_line = next;

				// If HTTP status code is 4xx or 5xx then close connection and try again 3 times
				if (strstr(cmd_line, "HTTP/1.1 4") || strstr(cmd_line, "HTTP/1.1 5")) {
					UI_bootScreen("The server appears to be having an issue (repo_check). Retrying");
					net_close(main_server);
					sleep(1);
				}

				if (strlen(cmd_line) == 1) {
					http_data = true;
				}

				if (http_data) {

					if (count >= 1) {
						if (count == 1) {
							strcpy(repo_list[repo_count].name, cmd_line);
						}

						if (count == 2) {
							strcpy(repo_list[repo_count].domain, cmd_line);
						}

						if (count == 3) {
							strcpy(repo_list[repo_count].list_file, cmd_line);
						}

						if (count == 4) {
							strcpy(repo_list[repo_count].apps_dir, cmd_line);
							repo_count++;
							count = 0;
						}
					}
					count++;
				}
			}
		}

		if (next != buf) { // some lines were processed
			offset = strlen(next);
			char tmp_buf[offset];
			memcpy(tmp_buf, next, offset);
			memcpy(buf, tmp_buf, offset);
		}
	}

	net_close(main_server);

	if (count >= 1) {
		UI_bootScreen("Repository list received");

		// Now check which server to use
		if (setting_repo != 0) {
			printf("Using repository: %s\n", repo_list[setting_repo].name);
			UI_bootScreen("Press '2' to revert to the default repository");
		}
	} else if (count == 0) {
		UI_bootScreen("Failed to receive repository list");
	}
}

// Write our message to the server
s32 write_http_reply(s32 server, char *msg) {
	u32 msglen = strlen(msg);
	char msgbuf[msglen + 1];
	if (msgbuf == NULL) return -ENOMEM;
	strcpy(msgbuf, msg);

	tcp_write (server, msgbuf, msglen);
	return 1;
}

bool tcp_write (const s32 s, char *buffer, const u32 length) {
	char *p;
	u32 step, left, block, sent;
	s32 res;

	step = 0;
	p = buffer;
	left = length;
	sent = 0;

	while (left) {

		block = left;
		if (block > 2048)
			block = 2048;

		res = net_write (s, p, block);

		if ((res == 0) || (res == -56)) {
			usleep (20 * 1000);
			continue;
		}

		if (res < 0) {
			break;
		}

		sent += res;
		left -= res;
		p += res;

		if ((sent / NET_BUFFER_SIZE) > step) {
			step++;
		}
	}

	return left == 0;
}


// Connect to the remote server
s32 server_connect(int repo_bypass) {

	struct sockaddr_in connect_addr;

	s32 server = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	memset(&connect_addr, 0, sizeof(connect_addr));
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = SOCKET_PORT;

	if (setting_repo == 0 || repo_bypass == 1) {
		if (hostname_ok) {
			if (!codemii_backup) {
				connect_addr.sin_addr.s_addr= getipbynamecached(MAIN_DOMAIN);
			} else {
				connect_addr.sin_addr.s_addr= getipbynamecached(FALLBACK_DOMAIN);
			}
		} else {
			if (!codemii_backup) {
				connect_addr.sin_addr.s_addr= inet_addr(IP_ADDRESS_OLD);
			} else {
				connect_addr.sin_addr.s_addr= inet_addr(IP_ADDRESS_OLD2);
			}
		}
	} else {
		connect_addr.sin_addr.s_addr = getipbynamecached(repo_list[setting_repo].domain);
	}

	if (net_connect(server, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) == -1) {
		net_close(server);
		die("Failed to connect to the remote server");
	}

	return server;
}

// Request the homebrew list
s32 request_list() {

	UI_bootScreen("Requesting Homebrew list");
	initialise_request();
	int retry = 0;
	timeout_counter = 0;
	while (list_received != true) {
		timeout_counter++;
		usleep(500000);
		if (timeout_counter > 240) {
			UI_bootScreen("Failed to receive the Homebrew list. Retrying");
			retry++;
			timeout_counter = 0;
			LWP_SuspendThread(request_thread);
			initialise_request();
		}
		if (retry >= 4) {
			die("Could not receive Homebrew list... returning you to HBC");
		}
	}

	FILE *f = NULL;
	if (setting_repo == 0) {
		if (setting_use_sd) {
			f = fopen ("sd:/apps/homebrew_browser_lite/listv036.txt", "rb");
		}
		else {
			f = fopen ("usb:/apps/homebrew_browser_lite/listv036.txt", "rb");
		}
	}
	else {
		if (setting_use_sd) {
			f = fopen ("sd:/apps/homebrew_browser_lite/external_repo_list.txt", "rb");
		}
		else {
			f = fopen ("usb:/apps/homebrew_browser_lite/external_repo_list.txt", "rb");
		}
	}

	// If file doesn't exist or can't open it then we can grab the latest file
	if (f == NULL) {
		UI_bootScreen("Homebrew list cannot be found");
		sleep(3);
		return -1;
	}
	// Open file and check file length for changes
	else {
		fseek (f , 0, SEEK_END);
		long file_check = ftell (f);
		rewind (f);

		if (file_check == 0) {
			if (setting_online) {
				UI_bootScreenTwo("Homebrew list is empty, retrying", "If this keeps happening, try reloading the Homebrew Browser");
			}
			else {
				die("Homebrew list file is empty. As you are in offline mode, HBB will now exit.\n Please go online to retreive the list file.");
			}
			sleep(3);
			return -1;
		}

		char cmd_line [5000];
		int array_count = 0;
		int line_number = -1;
		bool add_to_list = true;

		// Grab all directories and put them in a list
		int app_directories = 0;
		char apps_dir[80];
		strcpy(apps_dir, rootdir);
		strcat(apps_dir, "/apps");

		dir = opendir(apps_dir);

		if (dir != NULL) {
			// Grab directory listing
			char temp_path[MAXPATHLEN];
			while ((dent=readdir(dir)) != NULL) {
				strcpy(temp_path, apps_dir);
				strcat(temp_path, "/");
				strcat(temp_path, dent->d_name);
				stat(temp_path, &st);

				strcpy(foldername, dent->d_name);
				// Don't add homebrew sorter to the list
				if (S_ISDIR(st.st_mode) && strcmp(foldername,".") != 0 && strcmp(foldername,"..") != 0) {

					// Add folder to folder exists list
					int leng=strlen(foldername);
					int z;
					for(z=0; z<leng; z++)
						if (97<=foldername[z] && foldername[z]<=122)//a-z
							foldername[z]-=32;

					strcpy(folder_list[app_directories], foldername);
					app_directories++;
				}
			}
		}

		UI_bootScreen("Parsing Homebrew list");

		for (int i = 0; i < HOMEBREW_STRUCT_SIZE; i++) demos_list[i] = HOMEBREW_STRUCT_END;
		for (int i = 0; i < HOMEBREW_STRUCT_SIZE; i++) emulators_list[i] = HOMEBREW_STRUCT_END;
		for (int i = 0; i < HOMEBREW_STRUCT_SIZE; i++) games_list[i] = HOMEBREW_STRUCT_END;
		for (int i = 0; i < HOMEBREW_STRUCT_SIZE; i++) media_list[i] = HOMEBREW_STRUCT_END;
		for (int i = 0; i < HOMEBREW_STRUCT_SIZE; i++) utilities_list[i] = HOMEBREW_STRUCT_END;

		while (fgets (cmd_line, 2000, f)) {

			if (strstr(cmd_line, "Homebrew") && line_number == -1) {

				// Remote Homebrew Browser file size
				char *hb_size;
				hb_size = strtok (cmd_line, " "); // Removes the "Homebrew" part
				hb_size = strtok (NULL, " ");
				remote_hb_size = atoi(hb_size);

				// Update text
				char *split_tok = strtok (NULL, " ");

				int fd = 0;

				while (split_tok != NULL && (strcmp(split_tok,".") != 0)) {
					int ff;
					for (ff = 0; ff < strlen(split_tok); ff++) {
						update_text[fd] = split_tok[ff];
						fd++;
					}

					split_tok = strtok (NULL, " ");

					if (split_tok != NULL && (strcmp(split_tok,".") != 0)) {
						update_text[fd] = ' ';
						fd++;
					}
				}
				line_number = 0;
			}
			else {

				if (!add_to_list) {
					clear_list();
					add_to_list = true;
				}

				if (strstr(cmd_line, "=Games=") && line_number == 0) {
					int i;
					for (i = 0; i < array_count; i++) {
						games_list[i] = total_list_count - array_count + i;
					}
					array_count = 0;
					add_to_list = false;
				}
				else if (strstr(cmd_line, "=Emulators=") && line_number == 0) {
					int i;
					for (i = 0; i < array_count; i++) {
						emulators_list[i] = total_list_count - array_count + i;
					}
					array_count = 0;
					add_to_list = false;
				}
				else if (strstr(cmd_line, "=Media=") && line_number == 0) {
					int i;
					for (i = 0; i < array_count; i++) {
						media_list[i] = total_list_count - array_count + i;
					}
					array_count = 0;
					add_to_list = false;
				}
				else if (strstr(cmd_line, "=Utilities=") && line_number == 0) {
					int i;
					for (i = 0; i < array_count; i++) {
						utilities_list[i] = total_list_count - array_count + i;
					}
					array_count = 0;
					add_to_list = false;
				}
				else if (strstr(cmd_line, "=Demos=") && line_number == 0) {
					int i;
					for (i = 0; i < array_count; i++) {
						demos_list[i] = total_list_count - array_count + i;
					}
					array_count = 0;
					add_to_list = false;
				}

				if (add_to_list) {
					if (line_number == 0) {

						if (hbb_string_len == 0) {
							int z = 0;
							for (z = 0; z < strlen(cmd_line); z++) {
								if (cmd_line[z] == '\n' || cmd_line[z] == '\r') {
									hbb_string_len = strlen(cmd_line) - z;
									break;
								}
							}

						}

						char *split_tok;

						// Name
						split_tok = strtok (cmd_line, " ");

						// ftpii_ thing
						total_list[total_list_count].user_dirname[0] = 0;
						if (strcmp(split_tok,"ftpii") == 0) {
							strcpy(total_list[total_list_count].user_dirname,"ftpii");

							char apps_dir[80];
							strcpy(apps_dir, rootdir);
							strcat(apps_dir, "/apps");

							dir = opendir(apps_dir);
							if (dir != NULL) {
								char temp_path[MAXPATHLEN];
								while ((dent=readdir(dir)) != NULL) {
									strcpy(temp_path, apps_dir);
									strcat(temp_path, "/");
									strcat(temp_path, dent->d_name);
									stat(temp_path, &st);

									strcpy(filename, dent->d_name);
									if (S_ISDIR(st.st_mode)) {
										char * pch;
										pch = strstr (filename,"ftpii");

										if (pch != NULL) {
											int x;
											for (x = 0; x < strlen(filename); x++) {
												total_list[total_list_count].user_dirname[x] = filename[x];
											}
											total_list[total_list_count].user_dirname[strlen(filename)] = '\0';

											break;
										}

									}
								}
							}
							closedir(dir);
						}

						strcpy(total_list[total_list_count].name,split_tok);

						// App Time
						split_tok = strtok (NULL, " ");
						total_list[total_list_count].app_time = atoi(split_tok);

						// Img size
						split_tok = strtok (NULL, " ");
						total_list[total_list_count].img_size = atoi(split_tok);

						// Remote boot.dol/elf file size
						split_tok = strtok (NULL, " ");
						total_list[total_list_count].app_size = atoi(split_tok);

						// File extension, either .dol or .elf
						split_tok = strtok (NULL, " ");
						strcpy(total_list[total_list_count].boot_ext,split_tok);

						// Try to open the local boot.elf file if it exists
						total_list[total_list_count].local_app_size = 0;

						char boot_path[100] = "sd:/apps/";
						if (!setting_use_sd) {
							strcpy(boot_path,"usb:/apps/");
						}
						if (strcmp(total_list[total_list_count].name,"ftpii") == 0) {
							strcat(boot_path, total_list[total_list_count].user_dirname);
						}
						else {
							strcat(boot_path, total_list[total_list_count].name);
						}

						// New directory finding
						bool dir_exists = false;

						int g;
						for (g = 0; g < app_directories; g++) {
							strcpy(uppername, total_list[total_list_count].name);

							int leng=strlen(uppername);
								int z;
								for(z=0; z<leng; z++)
									if (97<=uppername[z] && uppername[z]<=122)//a-z
										uppername[z]-=32;

							if (strcmp(folder_list[g], uppername) == 0) {
								dir_exists = true;
								break;
							}
						}

						strcat(boot_path, "/boot.");
						strcat(boot_path, total_list[total_list_count].boot_ext);

						if (dir_exists) {
							FILE *f = fopen(boot_path, "rb");

							// Problems opening the file?
							if (f == NULL) {

								// Try opening the .elf file instead?
								char boot_path2[200] = "sd:/apps/";
								if (!setting_use_sd) {
									strcpy(boot_path2,"usb:/apps/");
								}

								if (strcmp(total_list[total_list_count].name,"ftpii") == 0) {
									strcat(boot_path2, total_list[total_list_count].user_dirname);
								}
								else {
									strcat(boot_path2, total_list[total_list_count].name);
								}

								strcat(boot_path2, "/boot.elf");

								FILE *f1 = fopen(boot_path2, "rb");

								if (f1 == NULL) {
									// Try opening the .dol.bak file instead?
									char boot_path2[200] = "sd:/apps/";
									if (!setting_use_sd) {
										strcpy(boot_path2,"usb:/apps/");
									}

									if (strcmp(total_list[total_list_count].name,"ftpii") == 0) {
										strcat(boot_path2, total_list[total_list_count].user_dirname);
									}
									else {
										strcat(boot_path2, total_list[total_list_count].name);
									}

									strcat(boot_path2, "/boot.dol.bak");

									FILE *f2 = fopen(boot_path2, "rb");

									if (f2 == NULL) {
										// Try opening the .elf file instead?
										char boot_path2[200] = "sd:/apps/";
										if (!setting_use_sd) {
											strcpy(boot_path2,"usb:/apps/");
										}

										if (strcmp(total_list[total_list_count].name,"ftpii") == 0) {
											strcat(boot_path2, total_list[total_list_count].user_dirname);
										}
										else {
											strcat(boot_path2, total_list[total_list_count].name);
										}

										strcat(boot_path2, "/boot.elf.bak");

										FILE *f3 = fopen(boot_path2, "rb");

										if (f3 == NULL) {
											// Try opening the .elf file instead?
											char boot_path3[200] = "sd:/apps/";
											if (!setting_use_sd) {
												strcpy(boot_path3,"usb:/apps/");
											}

											if (strcmp(total_list[total_list_count].name,"ftpii") == 0) {
												strcat(boot_path3, total_list[total_list_count].user_dirname);
											}
											else {
												strcat(boot_path3, total_list[total_list_count].name);
											}

											strcat(boot_path3, "/theme.zip");

											FILE *f4 = fopen(boot_path3, "rb");

											if (f4 == NULL) {
											}
											else {
											// Open file and get the file size
											fseek (f4 , 0, SEEK_END);
											total_list[total_list_count].local_app_size = ftell (f4);
											rewind (f4);
											fclose(f4);
											}
										}
										else {
											// Open file and get the file size
											fseek (f3 , 0, SEEK_END);
											total_list[total_list_count].local_app_size = ftell (f3);
											rewind (f3);
											fclose(f3);
											total_list[total_list_count].boot_bak = true;
										}
									}
									else {
										// Open file and get the file size
										fseek (f2 , 0, SEEK_END);
										total_list[total_list_count].local_app_size = ftell (f2);
										rewind (f2);
										fclose(f2);
										total_list[total_list_count].boot_bak = true;
									}
								}
								else {
									// Open file and get the file size
									fseek (f1, 0, SEEK_END);
									total_list[total_list_count].local_app_size = ftell (f1);
									rewind (f1);
									fclose(f);
								}
							}
							else {
								// Open file and get the file size
								fseek (f , 0, SEEK_END);
								total_list[total_list_count].local_app_size = ftell (f);
								rewind (f);
								fclose(f);
							}
						}

						// Total app size
						split_tok = strtok (NULL, " ");
						total_list[total_list_count].total_app_size = atoi(split_tok);

						// Downloads
						split_tok = strtok (NULL, " ");

						// Rating
						split_tok = strtok (NULL, " ");

						// Controllers
						split_tok = strtok (NULL, " ");
						strcpy(total_list[total_list_count].app_controllers, split_tok);

						// Folders to create (if any), a dot means no folders needed
						split_tok = strtok (NULL, " ");
						if (split_tok != NULL) {
							strcpy(total_list[total_list_count].folders, split_tok);
						}

						// Folders to not delete files from
						split_tok = strtok (NULL, " ");
						if (split_tok != NULL) {
							strcpy(total_list[total_list_count].folders_no_del, split_tok);
						}

						// Files to not extract
						split_tok = strtok (NULL, " ");
						if (split_tok != NULL) {
							strncpy(total_list[total_list_count].files_no_extract, split_tok, strlen(split_tok) - hbb_string_len);
							total_list[total_list_count].files_no_extract[strlen(split_tok) - hbb_string_len] = '\0';
						}

						line_number++;
					}


					// App name
					else if (line_number == 1) {
						if (hbb_string_len == 0) {
							int z = 0;
							for (z = 0; z < strlen(cmd_line); z++) {
								if (cmd_line[z] == '\n' || cmd_line[z] == '\r') {
									hbb_string_len = strlen(cmd_line) - z;
									break;
								}
							}

						}

						hbb_null_len = (strlen(cmd_line) - hbb_string_len + 1) > sizeof(total_list[total_list_count].app_name) ? sizeof(total_list[total_list_count].app_name) : strlen(cmd_line) - hbb_string_len + 1;

						strncpy(total_list[total_list_count].app_name, cmd_line, hbb_null_len);
						total_list[total_list_count].app_name[hbb_null_len - 1] = '\0';
						line_number++;
					}

					// Author
					else if (line_number == 2) {
						hbb_null_len = (strlen(cmd_line) - hbb_string_len + 1) > sizeof(total_list[total_list_count].app_author) ? sizeof(total_list[total_list_count].app_author) : strlen(cmd_line) - hbb_string_len + 1;

						strncpy(total_list[total_list_count].app_author, cmd_line, hbb_null_len);
						total_list[total_list_count].app_author[hbb_null_len - 1] = '\0';
						line_number++;
					}

					// Version
					else if (line_number == 3) {
						hbb_null_len = (strlen(cmd_line) - hbb_string_len + 1) > sizeof(total_list[total_list_count].app_version) ? sizeof(total_list[total_list_count].app_version) : strlen(cmd_line) - hbb_string_len + 1;

						strncpy(total_list[total_list_count].app_version, cmd_line, hbb_null_len);
						total_list[total_list_count].app_version[hbb_null_len - 1] = '\0';
						line_number++;
					}

					// Size
					else if (line_number == 4) {
						total_list[total_list_count].app_total_size = atoi(cmd_line);
						line_number++;
					}

					// Short Description
					else if (line_number == 5) {
						hbb_null_len = (strlen(cmd_line) - hbb_string_len + 1) > sizeof(total_list[total_list_count].app_short_description) ? sizeof(total_list[total_list_count].app_short_description) : strlen(cmd_line) - hbb_string_len + 1;

						strncpy(total_list[total_list_count].app_short_description, cmd_line, hbb_null_len);
						total_list[total_list_count].app_short_description[hbb_null_len - 1] = '\0';
						line_number++;
					}

					// Description
					else if (line_number == 6) {
						hbb_null_len = (strlen(cmd_line) - hbb_string_len + 1) > sizeof(total_list[total_list_count].app_description) ? sizeof(total_list[total_list_count].app_description) : strlen(cmd_line) - hbb_string_len;

						strncpy(total_list[total_list_count].app_description, cmd_line, hbb_null_len);
						total_list[total_list_count].app_description[hbb_null_len - 1] = '\0';
						total_list[total_list_count].original_pos = total_list_count;
						line_number = 0;
						total_list_count++;
						array_count++;
					}
				}
			}
		}
		fclose(f);
	}

	return 1;
}


// Request a file from the server and store it
s32 request_file(s32 server, FILE *f) {

	char message[NET_BUFFER_SIZE];
	s32 bytes_read = net_read(server, message, sizeof(message));

	int length_til_data = 0; // Count the length of each \n part until we reach actual data
	int tok_count = 2; // Count the number of \n tokens
	char *temp_tok;
	if (bytes_read == 0) { return -1; }
	temp_tok = strtok (message,"\n");

	while (temp_tok != NULL) {

		// If HTTP status code is 4xx or 5xx then close connection and try again 3 times
		if (strstr(temp_tok, "HTTP/1.1 4") || strstr(temp_tok, "HTTP/1.1 5")) {
			UI_bootScreen("The server appears to be having an issue (request_file). Retrying");
			return -1;
		}

		if (strlen(temp_tok) == 1) {
			break;
		}

		length_til_data += strlen(temp_tok);
		tok_count++;
		temp_tok = strtok (NULL, "\n");
	}

	// New place to store the real data
	char store_data[NET_BUFFER_SIZE];

	// We'll store this to the new array
	int q;
	int i = 0;
	for (q = length_til_data + tok_count; q < bytes_read; q++) {
		store_data[i] = message[q];
		i++;
	}

	// We now store the real data out of the first 1024 bytes
	if (store_data != NULL) {
		s32 bytes_written = fwrite(store_data, 1, i, f);
		if (bytes_written < i) {
			printf("DEBUG: fwrite error: [%i] %s\n", ferror(f), strerror(ferror(f)));
			sleep(1);
			return -1;
		}
	}

	if (hbb_updating) { progress_size = (int) (remote_hb_size / 29); }

	// Now we can continue storing the rest of the file
	while (bytes_read > 0) {
		bytes_read = net_read(server, message, sizeof(message));

		download_progress_counter += bytes_read;
		updating_current_size += bytes_read;

		// If updating the HBB, display dots as the progress bar
		if (hbb_updating) {
			int progress_count = (int) download_progress_counter / progress_size;

			if (progress_count > progress_number) {
				printf(".");
				progress_number = progress_count;
			}
		}

		s32 bytes_written = fwrite(message, 1, bytes_read, f);
		if (bytes_written < bytes_read) {
			timeout_counter = 0;
			printf("DEBUG: fwrite error: [%i] %s\n", ferror(f), strerror(ferror(f)));
			sleep(1);
			return -1;
		}

		if ((cancel_download && cancel_confirmed) || (hbb_updating && cancel_download)) {
			return -2;
		}
	}

	return 1;
}


s32 request_list_file(char *file_path, char *path) {

	s32 result = 0;
	s32 main_server;
	int retry_times = 0;

	FILE *f;

	// Try to get the file, if failed for a 3rd time, then return -1
	while (result != 1) {

		// Sleep for a little bit so we don't do all 3 requests at once
		if (retry_times > 1) {
			sleep(2);
		}
		else if (retry_times > 3) {
			sleep(3);
		}

		// Open file
		f = fopen(file_path, "wb");

		// If file can't be created
		if (f == NULL) {
			printf("There was a problem accessing the file %s.\n", path);
			return -1;
		}

		main_server = server_connect(0);

		char http_request[1000];
		strcpy(http_request, "GET ");
		strcat(http_request, path);

		strcat(http_request, " HTTP/1.0\r\nHost: ");
		if (setting_repo == 0) {
			if (!codemii_backup) {
				strcat(http_request, MAIN_DOMAIN);
			} else {
				strcat(http_request, FALLBACK_DOMAIN);
			}
		} else {
			strcat(http_request, repo_list[setting_repo].domain);
		}
		strcat(http_request, "\r\nCache-Control: no-cache\r\n\r\n");

		write_http_reply(main_server, http_request);
		result = request_file(main_server, f);

		retry_times++;

		fclose(f);
		net_close(main_server);

		// User cancelled download
		if (result == -2) {
			return -1;
		}

		if (retry_times >= 6) {
			return -1;
		}
	}

	return 1;
}

// Create the path to the folder if needed, create the file and request the file from the server
s32 create_and_request_file(char* path1, char* name, char *filename) {
	char appname[100];
	if (!strcmp(name, "homebrew_browser_lite")) strcpy(appname, "homebrew_browser");
	else strcpy(appname, name);

	// Path
	char path[300];
	strcpy(path, path1);
	strcat(path, name);

	// Create the folder if it's a request on a directory
	if (strcmp(filename, ".png") != 0) {
		if (!opendir(path)) {
			mkdir(path, 0777);
			if (!opendir(path)) {
				printf("Could not create %s directory.\n", path);
				return -1;
			}
		}
	}

	strcat(path, filename);


	s32 result = 0;
	s32 main_server;
	int retry_times = 0;

	FILE *f;

	// Try to get the file, if failed for a 3rd time, then return -1
	while (result != 1) {

		// Sleep for a little bit so we don't do all 3 requests at once
		if (retry_times > 1) {
			sleep(2);
		}
		else if (retry_times > 3) {
			sleep(3);
		}

		// Open file
		f = fopen(path, "wb");

		// If file can't be created
		if (f == NULL) {
			printf("There was a problem accessing the file %s.\n", path);
			return -1;
		}

		main_server = server_connect(0);

		char http_request[1000];
		if (setting_repo == 0) {
			strcpy(http_request, "GET /hbb/");
			if (strstr(appname,"ftpii")) {
				strcat(http_request, "ftpii");
			}
			else {
				strcat(http_request, appname);
			}
		}
		else {
			strcpy(http_request, "GET ");
			strcat(http_request, repo_list[setting_repo].apps_dir);
			strcat(http_request, appname);
		}

		strcat(http_request, filename);
		strcat(http_request, " HTTP/1.0\r\nHost: ");
		if (setting_repo == 0) {
			if (!codemii_backup) {
				strcat(http_request, MAIN_DOMAIN);
			}
			else {
				strcat(http_request, FALLBACK_DOMAIN);
			}
		}
		else {
			strcat(http_request, repo_list[setting_repo].domain);
		}
		strcat(http_request, "\r\nCache-Control: no-cache\r\n\r\n");

		write_http_reply(main_server, http_request);
		result = request_file(main_server, f);

		retry_times++;

		fclose(f);
		net_close(main_server);

		// User cancelled download
		if (result == -2) {
			return -1;
		}

		if (retry_times >= 6) {
			return -1;
		}
	}

	return 1;
}
