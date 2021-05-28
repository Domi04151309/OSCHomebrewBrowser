#ifndef _ACTIVITIES_H_
#define _ACTIVITIES_H_

#include <stdio.h>
#include <stdlib.h>

#define ACTIVITY_MAIN 1
#define ACTIVITY_APP 2

#define ACTIVITY_ABOUT 3
#define ACTIVITY_HELP_CONTROLLER 4

#define ACTIVITY_MENU 5

#define ACTIVITY_SETTINGS 6

#define ACTIVITIES_STACK_SIZE 16

struct stack {
  int size;
  int top;
  uint8_t *items;
};

void ACTIVITIES_open(uint8_t activity);
uint8_t ACTIVITIES_current();
void ACTIVITIES_goBack();

#endif
