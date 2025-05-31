#ifndef LIBPSX_H
#define LIBPSX_H

// Width of screen in pixels
#define PSX_SCREEN_WIDTH_PX 640

// Height of screen in pixels
#define PSX_SCREEN_HEIGHT_PX 240

// Width of screen in characters
#define PSX_TEXT_COLS (PSX_SCREEN_WIDTH_PX / 8)

// Height of screen in characters
#define PSX_TEXT_LINES (PSX_SCREEN_HEIGHT_PX / 8 - 2)

//
// STUBS
//

// <stdio.h>
#define EOF (-1)
#define fclose(f)
#define feof(f) 0
#define fgetc(f) EOF
#define fgets(s, size, f) NULL
#define fopen(path, mode) NULL
#define fprintf(f, fmt, ...)
#define fread(ptr, size, nmemb, f) 0
#define fwrite(data, size, count, f) 0
#define rewind(f)

// <stdlib.h>
#define getenv(name) NULL

// <unistd.h>
#define unlink(path) 0

// tags.cc
#define write2(f, buffer, count) 0
#define read2(f, buffer, count) 0
#define marshallByte(th, data)
#define marshallShort(th, data)
#define marshallLong(th, data)
#define marshallBoolean(th, data)
#define marshallString(th, data)

#define unmarshallByte(th) 0
#define unmarshallShort(th) 0
#define unmarshallFloat(th) 0
#define unmarshallBoolean(th) 0
#define unmarshallString(th, data, maxSize)

#define tag_init(...)
#define tag_construct(th, i)
#define tag_write(th, saveFile)
#define tag_set_expected(tags, fileType)
#define tag_missing(tag, minorVersion)
#define tag_read(fp, minorVersion) 0

//
// psx/curses.cc
//
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

//
// psx/psx.cc
//

// Initialize the PSX subsystem
void init_psx();

// Update the PSX subsystem
void update_psx();

//
// psx/util.cc
//

// Wait for a specified amount of milliseconds
void delay(int ms);

// Halt execution with provided exit code
void exit(int code);

#endif
