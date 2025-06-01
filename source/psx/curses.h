#ifndef PSX_CURSES_H
#define PSX_CURSES_H

#define _setcursortype(ty)
#define getch() 0
#define window(x,y,w,h)
#define kbhit() 0

extern char psx_text_buffer[PSX_TEXT_LINES][PSX_TEXT_COLS + 1];

extern void cprintf(const char *format, ...);

extern void clrscr();

extern void gotoxy(int x, int y);

extern void putch(unsigned char ch);

extern void textcolor(int col);

extern void textbackground(int col);

extern int wherex();

extern int wherey();

#endif