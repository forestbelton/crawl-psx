#include "libpsx.h"
#include "enum.h"

#include <cassert>
#include <cstdarg>

static int cursor_x = 0;
static int cursor_y = 0;
static volatile int ch = CMD_NO_CMD;
static int fg = DEFAULT_FG_COLOR;

psx_text_cell psx_text_buffer[PSX_TEXT_LINES][PSX_TEXT_COLS];

void set_input_cmd(const int cmd) {
    ch = cmd;
}

int get_input_cmd() {
    while (ch == CMD_NO_CMD) {
        update_psx();
    }

    const int result = ch;
    ch = CMD_NO_CMD;

    return result;
}

int getch() {
    while (ch == CMD_NO_CMD) {
        update_psx();
    }

    const int result = ch;
    ch = CMD_NO_CMD;

    return result;
}

void clrscr() {
    for (auto &row: psx_text_buffer) {
        for (auto &cell: row) {
            cell = {' ', DEFAULT_FG_COLOR};
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void putch(char ch) {
    // NB: viewwindow writes a lot of 0s. ncurses writes a space, so we do too
    if (ch == '\000') {
        ch = ' ';
    }

    if (ch == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y == PSX_TEXT_LINES) {
            cursor_y--;
        }
    } else {
        psx_text_buffer[cursor_y][cursor_x++] = {ch, fg};
    }

    if (cursor_x == PSX_TEXT_COLS) {
        if (cursor_y != PSX_TEXT_LINES - 1) {
            cursor_y++;
            cursor_x = 0;
        } else {
            cursor_x--;
        }
    }
}

void gotoxy(const int x, const int y) {
    if (x < 1 || x > PSX_TEXT_COLS || y < 1 || y > PSX_TEXT_LINES) {
        return;
    }
    cursor_x = x - 1;
    cursor_y = y - 1;
}

void textcolor(const int col) {
    assert(col >= 0 && col <= 0xf);
    fg = col & 0xf;
}

void textbackground([[maybe_unused]] int col) {
    // NB: This isn't ever used except to set the background to its default
    // color. To save on redundant TILE primitives we skip the implementation.
}

int wherex() {
    return cursor_x + 1;
}

int wherey() {
    return cursor_y + 1;
}

void cprintf(const char *format, ...) {
    static char buffer[2048]; // One full screen if no control seq...

    va_list argp;
    va_start(argp, format);
    vsprintf(buffer, format, argp);
    va_end(argp);

    for (int i = 0; buffer[i] != 0; i++) {
        putch(buffer[i]);
    }
}

void cputs(const char *str) {
    while (*str != 0) {
        putch(*str);
        str++;
    }
}
