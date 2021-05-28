#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdio.h>
#include <stdbool.h>

extern uint8_t menu_section;
extern bool select_repo;
extern bool select_category;
extern bool select_sort;

void SETTINGS_render();

#endif
