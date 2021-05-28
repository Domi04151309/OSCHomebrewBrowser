#include "activities.h"
#include <stdio.h>
#include <stdlib.h>

struct stack *activityStack = NULL;

static struct stack* newStack(int capacity) {
  struct stack *pt = (struct stack*)malloc(sizeof(struct stack));

  pt->size = capacity;
  pt->top = -1;
  pt->items = (uint8_t*)malloc(sizeof(uint8_t) * capacity);

  return pt;
}

static int isEmpty(struct stack *pt) {
  return pt->top == -1;
}

static int isFull(struct stack *pt) {
  return pt->top == pt->size - 1;
}

static void push(struct stack *pt, uint8_t x) {
  if (isFull(pt)) exit(EXIT_FAILURE);
  pt->items[++pt->top] = x;
}

static uint8_t peek(struct stack *pt) {
  if (isEmpty(pt)) return -1;
  else return pt->items[pt->top];
}

static uint8_t pop(struct stack *pt) {
  if (isEmpty(pt)) exit(EXIT_FAILURE);
  return pt->items[pt->top--];
}

void ACTIVITIES_open(uint8_t activity) {
  if (activityStack == NULL) activityStack = newStack(ACTIVITIES_STACK_SIZE);
  if (peek(activityStack) == activity) return;
  if (isFull(activityStack)) {
    struct stack *tempStack = newStack(ACTIVITIES_STACK_SIZE / 2);
    for (uint8_t i = 0; i < ACTIVITIES_STACK_SIZE / 2; i++) {
      push(tempStack, pop(activityStack));
    }
    activityStack = newStack(ACTIVITIES_STACK_SIZE);
    for (uint8_t i = 0; i < ACTIVITIES_STACK_SIZE / 2; i++) {
      push(activityStack, pop(tempStack));
    }
    free(tempStack);
  }
  push(activityStack, activity);
}

uint8_t ACTIVITIES_current() {
  if (activityStack == NULL) activityStack = newStack(ACTIVITIES_STACK_SIZE);
  if (!isEmpty(activityStack)) return peek(activityStack);
  else return ACTIVITY_MAIN;
}

void ACTIVITIES_goBack() {
  if (activityStack == NULL) activityStack = newStack(ACTIVITIES_STACK_SIZE);
  if (!isEmpty(activityStack)) pop(activityStack);
}
