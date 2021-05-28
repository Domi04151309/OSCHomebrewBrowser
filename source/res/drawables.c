#include "drawables.h"

#include "../GRRLIB/GRRLIB.h"

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
#include "date_png.h"
#include "app_question_png.h"
#include "app_tick_png.h"
#include "app_tick_small_png.h"
#include "stack_png.h"
#include "run_png.h"
#include "app_new_png.h"
#include "list_png.h"
#include "download_png.h"
#include "sort_arrow_down_png.h"
#include "sort_arrow_up_png.h"
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

GRRLIB_texImg *mouse_img;

GRRLIB_texImg *control_wiimote_img;
GRRLIB_texImg *control_wiimote_2_img;
GRRLIB_texImg *control_wiimote_3_img;
GRRLIB_texImg *control_wiimote_4_img;
GRRLIB_texImg *control_nunchuck_img;
GRRLIB_texImg *control_classic_img;
GRRLIB_texImg *control_gcn_img;
GRRLIB_texImg *control_keyboard_img;
GRRLIB_texImg *control_zapper_img;
GRRLIB_texImg *logo_img;
GRRLIB_texImg *date_img;
GRRLIB_texImg *name_img;
GRRLIB_texImg *stack_img;
GRRLIB_texImg *run_img;
GRRLIB_texImg *app_question_img;
GRRLIB_texImg *app_tick_img;
GRRLIB_texImg *app_tick_small_img;
GRRLIB_texImg *app_new_img;
GRRLIB_texImg *list_img;
GRRLIB_texImg *download_img;
GRRLIB_texImg *sort_arrow_down_img;
GRRLIB_texImg *sort_arrow_up_img;

GRRLIB_texImg *help_controller_img;

GRRLIB_texImg *gear_bg_img;
GRRLIB_texImg *home_bg_img;
GRRLIB_texImg *app_cross_img;

GRRLIB_texImg *cancel_download_prompt_img;
GRRLIB_texImg *button_no_img;
GRRLIB_texImg *button_no_highlight_img;
GRRLIB_texImg *button_yes_img;
GRRLIB_texImg *button_yes_highlight_img;
GRRLIB_texImg *updated_close_img;
GRRLIB_texImg *updated_close_highlight_img;
GRRLIB_texImg *apps_repo_img;
GRRLIB_texImg *apps_start_cat_img;
GRRLIB_texImg *apps_start_sort_img;
GRRLIB_texImg *next_img;
GRRLIB_texImg *prev_img;

void DRAWABLES_load() {
  mouse_img = GRRLIB_LoadTexture(mouse_png);

	control_wiimote_img = GRRLIB_LoadTexture(control_wiimote_png);
	control_wiimote_2_img = GRRLIB_LoadTexture(control_wiimote_2_png);
	control_wiimote_3_img = GRRLIB_LoadTexture(control_wiimote_3_png);
	control_wiimote_4_img = GRRLIB_LoadTexture(control_wiimote_4_png);
	control_nunchuck_img = GRRLIB_LoadTexture(control_nunchuck_png);
	control_classic_img = GRRLIB_LoadTexture(control_classic_png);
	control_gcn_img = GRRLIB_LoadTexture(control_gcn_png);
	control_keyboard_img = GRRLIB_LoadTexture(control_keyboard_png);
	control_zapper_img = GRRLIB_LoadTexture(control_zapper_png);
	logo_img = GRRLIB_LoadTexture(logo_png);
	date_img = GRRLIB_LoadTexture(date_png);
	name_img = GRRLIB_LoadTexture(name_png);
	stack_img = GRRLIB_LoadTexture(stack_png);
	run_img = GRRLIB_LoadTexture(run_png);
	app_question_img = GRRLIB_LoadTexture(app_question_png);
	app_tick_img = GRRLIB_LoadTexture(app_tick_png);
	app_tick_small_img = GRRLIB_LoadTexture(app_tick_small_png);
	app_new_img = GRRLIB_LoadTexture(app_new_png);
	list_img = GRRLIB_LoadTexture(list_png);
	download_img = GRRLIB_LoadTexture(download_png);
	sort_arrow_down_img = GRRLIB_LoadTexture(sort_arrow_down_png);
	sort_arrow_up_img = GRRLIB_LoadTexture(sort_arrow_up_png);

	help_controller_img = GRRLIB_LoadTexture(help_controller_png);

	gear_bg_img = GRRLIB_LoadTexture(gear_bg_png);
	home_bg_img = GRRLIB_LoadTexture(home_bg_png);
	app_cross_img = GRRLIB_LoadTexture(app_cross_png);

	cancel_download_prompt_img = GRRLIB_LoadTexture(cancel_download_prompt_png);
	button_no_img = GRRLIB_LoadTexture(button_no_png);
	button_no_highlight_img = GRRLIB_LoadTexture(button_no_highlight_png);
	button_yes_img = GRRLIB_LoadTexture(button_yes_png);
	button_yes_highlight_img = GRRLIB_LoadTexture(button_yes_highlight_png);
	updated_close_img = GRRLIB_LoadTexture(updated_close_png);
	updated_close_highlight_img = GRRLIB_LoadTexture(updated_close_highlight_png);
	apps_repo_img = GRRLIB_LoadTexture(apps_repo_png);
	apps_start_cat_img = GRRLIB_LoadTexture(apps_start_cat_png);
	apps_start_sort_img = GRRLIB_LoadTexture(apps_start_sort_png);
	next_img = GRRLIB_LoadTexture(next_png);
	prev_img = GRRLIB_LoadTexture(prev_png);
}
