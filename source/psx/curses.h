#ifndef PSX_CURSES_H
#define PSX_CURSES_H

#include <cstdint>

#define _setcursortype(ty)
#define window(x,y,w,h)
#define kbhit() 0

constexpr int DEFAULT_FG_COLOR = 15;
constexpr int DEFAULT_BG_COLOR = 0;

struct psx_text_cell {
    unsigned char ch;
    int fg;
};

extern psx_text_cell psx_text_buffer[PSX_TEXT_LINES][PSX_TEXT_COLS];

extern void set_pad_btn(uint32_t btn);

extern uint32_t getpad();

extern void set_input_cmd(int cmd);

extern int getch();

extern int get_input_cmd();

extern void cprintf(const char *format, ...);

extern void cputs(const char *str);

extern void clrscr();

extern void gotoxy(int x, int y);

extern void putch(unsigned char ch);

extern void textcolor(int col);

extern void textbackground(int col);

extern int wherex();

extern int wherey();

#endif