#pragma once

#include <setjmp.h>
#include "list.h"

#define TASK_YIELD()                      \
    do {                                  \
        if (setjmp(cur_task->env) == 0) { \
            task_switch();                \
        }                                 \
    } while (0)

#define TASK_SCHEDULE()                   \
    do {                                  \
        if (setjmp(cur_task->env) == 0) { \
            task_schedule();              \
        }                                 \
    } while (0)

extern struct task *cur_task;

struct task {
    jmp_buf env;
    struct list_head list;
    void (*func)(void);
};

void task_schedule();

void schedule(void);

void task_register(void (*func)(void));

void task_degister(const void (*const func)(void));

void task_add(struct task *task);

void task_switch();
