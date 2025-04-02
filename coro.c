#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coro.h"

static jmp_buf sched;

LIST_HEAD(tasklist);

struct task *cur_task = NULL;

void task_register(void (*func)(void))
{
    struct task *task = malloc(sizeof(struct task));
    if (!task) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(task, 0, sizeof(struct task));
    task->func = func;

    task_add(task);
}

void task_degister(void (*const func)(void))
{
    struct task *task = NULL;
    if (list_empty)
        list_for_each_entry(task, &tasklist, list) {
            if (task->func == func) {
                list_del(&task->list);
                free(task);
                break;
            }
        }
}

void task_add(struct task *task)
{
    list_add(&task->list, &tasklist);
}

void task_switch()
{
    if (!list_empty(&tasklist)) {
        struct task *t = list_first_entry(&tasklist, struct task, list);
        list_del(&t->list);
        list_add_tail(&t->list, &tasklist);
        cur_task = t;
        longjmp(t->env, 1);
    }
}

void task_schedule()
{
    longjmp(sched, 1);
}

void schedule(void)
{
    setjmp(sched);

    static struct list_head *cur = &tasklist;
    cur = cur->next;
    struct task *task = list_entry(cur, struct task, list);

    while (cur != &tasklist) {
        cur_task = task;
        task->func();
        printf("Never reached\n");
    }

    task_switch();
}