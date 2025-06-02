#include <cassert>

#include "libpsx.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "enum.h"

static int cursor_x = 0;
static int cursor_y = 0;

// Input buffer
static int ch;

static int fg = DEFAULT_FG_COLOR;

psx_text_cell psx_text_buffer[PSX_TEXT_LINES][PSX_TEXT_COLS];

void set_input_cmd(const int _ch) {
    ch = _ch;
}

int get_input_cmd() {
    const int old_ch = ch;
    ch = CMD_NO_CMD;
    return old_ch;
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
    if (x < 0 || x > PSX_TEXT_COLS || y < 0 || y > PSX_TEXT_LINES) {
        return;
    }
    cursor_x = x;
    cursor_y = y;
}

void textcolor(const int col) {
    assert(col >= 0 && col <= 0xf);
    fg = col & 0xf;
}

void textbackground([[maybe_unused]] int col) {
    // TODO(forest): Set bg
}

int wherex() {
    return cursor_x;
}

int wherey() {
    return cursor_y;
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
