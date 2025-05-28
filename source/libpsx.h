#ifndef LIBPSX_H
#define LIBPSX_H

// File I/O
#define fclose(f)
#define feof(f) 0
#define fopen(path, mode) NULL
#define fread(ptr, size, nmemb, f) 0
#define fwrite(data, size, count, f) 0
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

// actual things we plan to implement
void delay(int ms);
void exit(int code);

#endif
