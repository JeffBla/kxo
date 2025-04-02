#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "coro.h"
#include "game.h"

#define XO_STATUS_FILE "/sys/module/kxo/initstate"
#define XO_DEVICE_FILE "/dev/kxo"
#define XO_DEVICE_ATTR_FILE "/sys/class/kxo/kxo/kxo_state"

extern struct task *cur_task;

static bool status_check(void)
{
    FILE *fp = fopen(XO_STATUS_FILE, "r");
    if (!fp) {
        printf("kxo status : not loaded\n");
        return false;
    }

    char read_buf[20];
    fgets(read_buf, 20, fp);
    read_buf[strcspn(read_buf, "\n")] = 0;
    if (strcmp("live", read_buf)) {
        printf("kxo status : %s\n", read_buf);
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

static struct termios orig_termios;

static void raw_mode_disable(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void raw_mode_enable(void)
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(raw_mode_disable);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static bool read_attr, end_attr;

static void listen_keyboard_handler(void)
{
    TASK_SCHEDULE();
    while (!end_attr) {
        int attr_fd = open(XO_DEVICE_ATTR_FILE, O_RDWR);
        char input;

        if (read(STDIN_FILENO, &input, 1) == 1) {
            char buf[20];
            switch (input) {
            case 9: /* Ctrl-I */
                read(attr_fd, buf, 6);
                buf[4] = '0';
                write(attr_fd, buf, 6);
                printf("Starting the kernel space tic-tac-toe game...\n");
                break;
            case 15: /* Ctrl-O */
                read(attr_fd, buf, 6);
                buf[4] = '1';
                read_attr = false;
                end_attr = true;
                write(attr_fd, buf, 6);
                printf("Stopping the kernel space tic-tac-toe game...\n");
                break;
            case 16: /* Ctrl-P */
                read(attr_fd, buf, 6);
                buf[0] = (buf[0] - '0') ? '0' : '1';
                read_attr ^= 1;
                write(attr_fd, buf, 6);
                if (!read_attr)
                    printf("Stopping to display the chess board...\n");
                break;
            case 17: /* Ctrl-Q */
                read(attr_fd, buf, 6);
                buf[4] = '1';
                read_attr = false;
                end_attr = true;
                write(attr_fd, buf, 6);
                printf("Stopping the kernel space tic-tac-toe game...\n");
                break;
            }
        }
        close(attr_fd);

        TASK_YIELD();
    }
}

static int device_fd;
static char display_buf[DRAWBUFFER_SIZE];
static char table[N_GRIDS];

/* Draw the board into display_buf */
static int drawing(char *table)
{
    int i = 0, k = 0;
    display_buf[i++] = '\n';
    display_buf[i++] = '\n';

    while (i < DRAWBUFFER_SIZE) {
        for (int j = 0; j < (BOARD_SIZE << 1) - 1 && k < N_GRIDS; j++) {
            display_buf[i++] = j & 1 ? '|' : table[k++];
        }
        display_buf[i++] = '\n';
        for (int j = 0; j < (BOARD_SIZE << 1) - 1; j++) {
            display_buf[i++] = '-';
        }
        display_buf[i++] = '\n';
    }

    return 0;
}
static void draw_board(void)
{
    TASK_SCHEDULE();
    while (!end_attr) {
        if (read_attr) {
            printf("\033[H\033[J"); /* ASCII escape code to clear the screen */
            read(device_fd, table, N_GRIDS);
            drawing(table);
            // avoid flickering
            usleep(100);
            printf("%s", display_buf);
        }
        TASK_YIELD();
    }
}

int main(int argc, char *argv[])
{
    if (!status_check())
        exit(1);

    raw_mode_enable();
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    device_fd = open(XO_DEVICE_FILE, O_RDONLY);
    int device_flags = fcntl(device_fd, F_GETFL, 0);
    fcntl(device_fd, F_SETFL, device_flags | O_NONBLOCK);
    read_attr = true;
    end_attr = false;

    task_register(listen_keyboard_handler);
    task_register(draw_board);

    schedule();

    task_degister(listen_keyboard_handler);
    task_degister(draw_board);

    raw_mode_disable();
    fcntl(STDIN_FILENO, F_SETFL, flags);

    close(device_fd);

    return 0;
}
