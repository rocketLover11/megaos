#pragma once
#include "thread.h"

void scheduler_init();
void scheduler_add_thread(Thread *t);
void schedule();
Thread *scheduler_current();