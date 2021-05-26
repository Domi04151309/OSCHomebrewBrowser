#ifndef _ACTIVITIES_H_
#define _ACTIVITIES_H_

#include <stdio.h>
#include <stdlib.h>

#define ACTIVITY_MAIN 1
#define ACTIVITY_APP 2

#define ACTIVITY_INFO 3

#define ACTIVITY_ABOUT 4
#define ACTIVITY_HELP_CONTROLLER 5

#define ACTIVITY_MENU 6

#define ACTIVITY_SETTINGS 7

#define ACTIVITIES_STACK_SIZE 16

struct stack {
  int size;
  int top;
  uint8_t *items;
};

struct stack* newStack(int capacity);
int size(struct stack *pt);
int isEmpty(struct stack *pt);
int isFull(struct stack *pt);
void push(struct stack *pt, uint8_t x);
uint8_t peek(struct stack *pt);
uint8_t pop(struct stack *pt);

void ACTIVITIES_open(uint8_t activity);
uint8_t ACTIVITIES_current();
void ACTIVITIES_goBack();

#endif
