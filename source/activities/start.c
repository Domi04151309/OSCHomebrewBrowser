#include "start.h"

#include <stdio.h>
#include <unistd.h>
#include "../common.h"
#include "../ui.h"

void START_tryWWW() {
  int main_retries = 0;
  while (!www_passed && main_retries < 3) {
    initialise_www();
    int waiting = 0;
    while (!www_passed && waiting < 5) {
      sleep(1);
      waiting++;
    }
    if (!www_passed) UI_bootScreen("Failed, retrying");
    main_retries++;
    suspend_www_thread();
  }
}
