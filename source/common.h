/*

Homebrew Browser - a simple way to view and install Homebrew releases via the Wii

Author: teknecal & Domi04151309

Using some source from ftpii v0.0.5
ftpii Source Code Copyright (C) 2008 Joseph Jordan <joe.ftpii@psychlaw.com.au>

*/
#ifndef _COMMON_H_
#define _COMMON_H_

#include <ogcsys.h>
#include <stdio.h>
#include "GRRLIB/GRRLIB.h"

#define HOMEBREW_STRUCT_SIZE 256
#define HOMEBREW_STRUCT_END (HOMEBREW_STRUCT_SIZE - 1)

extern const char *CRLF;
extern const u32 CRLF_LENGTH;
extern int xfb_height;

extern char rootdir[10];

// Zip
extern int zip_size;
extern long zip_progress;
extern long extract_part_size;
extern bool cancel_extract;
extern bool hbb_updating;
extern int unzip_file_counter;
extern int unzip_file_count;
extern char no_unzip_list[10][300];
extern int no_unzip_count;

u8 initialise_reset_button();
u8 initialise_www();
u8 load_icons();
u8 initialise_download();
u8 initialise_delete();
u8 initialise_request();

void exitApp(int code);

void suspend_reset_thread();
void suspend_www_thread();
void die(char *msg);
void initialise();
void SetScreen();
void clearscreen();
bool initialise_device();
bool test_fat();
void initialise_fat();

s32 write_exact(s32 s, char *buf, s32 length);
s32 read_to_file(s32 s, FILE *f);
u32 split(char *s, char sep, u32 maxsplit, char *result[]);

void initialise_network();
bool check_wifi();
extern u32 net_gethostip();

extern bool codemii_backup;
extern bool www_passed;

extern int category_old_selection;

extern int download_in_progress;
extern int extract_in_progress;
extern int delete_in_progress;
extern int selected_app;
extern int total_list_count;
extern int timeout_counter;

extern int download_part_size;
extern long download_progress_counter;
extern int download_progress_number;
extern long remote_hb_size;
extern char update_text[1000];

extern char temp_name[100];

extern int error_number;
extern bool cancel_download;
extern bool cancel_delete;

extern bool sd_card_update;
extern long long sd_card_free;

extern int download_icon;

extern bool setting_sd_card;
extern bool setting_hide_installed;
extern bool setting_online;
extern bool setting_rumble;
extern bool setting_update_icon;
extern bool setting_tool_tip;
extern char setting_last_boot[14];
extern bool setting_use_sd;
extern int setting_repo;
extern int setting_sort;
extern int setting_category;
extern bool setting_disusb;
extern bool setting_wiiside;

extern bool cancel_confirmed;

extern bool downloading_icons;

void update_settings();
void load_settings();
void load_mount_settings();

void hide_apps_installed();
void sort_by_date(bool min_to_max);
void sort_by_name (bool min_to_max);
extern int sort_up_down;

extern int updating;
extern int new_updating;
extern int updating_total_size;
extern int updating_part_size;
extern long updating_current_size;
extern bool changing_cat;
extern bool download_icon_sleeping;
extern int repo_count;

extern int update_xml;
extern char testy[200];

extern bool list_received;
extern int no_manage_count;

void download_queue_size();
bool check_server();
void initialise_codemii();
void initialise_codemii_backup();

s32 write_http_reply(s32 server, char *msg);
s32 server_connect(int repo_bypass);
s32 request_list();
s32 request_file(s32 server, FILE *f);
s32 create_and_request_file(char* path1, char* appname, char *filename);
bool tcp_write (const s32 s, char *buffer, const u32 length);
s32 request_list_file(char *file_path, char *path);

int load_file_to_memory(const char *filename, unsigned char **result);
void clear_list();
void clear_temp_list();
void clear_store_list();
int remove_file(char* path);
int remove_dir(char* path);
int delete_dir_files(char* path);
int create_dir(char* path);
bool unzipArchive(char * zipfilepath, char * unzipfolderpath);
void check_temp_files();
void save_xml_name();
void copy_xml_name();
void repo_check();
void add_to_log(char* text, ...);

struct homebrew_struct {
	char name[100];
	long app_size;
	long app_time;
	long img_size;
	long local_app_size;
	long total_app_size;
	int in_download_queue;
	char user_dirname[100];
	char folders[1000];
	char folders_no_del[400];
	char files_no_extract[200];
	char boot_ext[10];
	bool boot_bak;
	bool no_manage;

	// About
	bool about_loaded;
	char app_name[100];
	char app_short_description[100];
	char app_description[500];
	char app_author[100];
	char app_version[20];
	int app_total_size;
	char app_controllers[20];

	// Icon image
	int file_found;
	unsigned char *content;

	// Management
	int original_pos;
};

struct repo_struct {
	char name[200];
	char domain[100];
	char list_file[200];
	char apps_dir[100];
};

extern struct repo_struct repo_list[200];

// List to show
extern int current_items[HOMEBREW_STRUCT_SIZE];

extern int emulators_list[HOMEBREW_STRUCT_SIZE];
extern int games_list[HOMEBREW_STRUCT_SIZE];
extern int media_list[HOMEBREW_STRUCT_SIZE];
extern int utilities_list[HOMEBREW_STRUCT_SIZE];
extern int demos_list[HOMEBREW_STRUCT_SIZE];

// Total list
extern struct homebrew_struct total_list[HOMEBREW_STRUCT_SIZE];

// Temp list
extern int temp_list[HOMEBREW_STRUCT_SIZE];

// Temp list to use to download/extract/delete
extern struct homebrew_struct job_store;

int array_length(struct homebrew_struct array[]);
int int_array_length(int array[]);

#endif /* _COMMON_H_ */
