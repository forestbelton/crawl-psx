#ifndef LIBPSX_H
#define LIBPSX_H

// File I/O
#define fclose(f)
#define feof(f) 0
#define fopen(path, mode) NULL
#define fwrite(data, size, count, f)
#define rewind(f)
#define unlink(path) 0

// NCurses
#define _setcursortype(ty)
#define gotoxy(x,y)
#define textcolor(col)
#define cprintf(fmt,...)
#define clrscr()
#define wherex() 0
#define wherey() 0
#define getch() 0
#define putch(ch)
#define textbackground(n)
#define window(x,y,w,h)
#define kbhit() 0

void delay(int ms);
void exit(int code);

#endif
