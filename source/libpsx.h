#ifndef LIBPSX_H
#define LIBPSX_H

// Width of screen in pixels
constexpr int PSX_SCREEN_WIDTH_PX = 640;

// Height of screen in pixels
constexpr int PSX_SCREEN_HEIGHT_PX = 240;

// Width of screen in characters
constexpr int PSX_TEXT_COLS = 80;

// Height of screen in characters
constexpr int PSX_TEXT_LINES = 25;

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

#include "psx/curses.h"

#define C_SQUARE 0x80
#define C_TRIANGLE 0x81
#define C_CIRCLE 0x82
#define C_CROSS 0x83

#define S_SQUARE "\x80"
#define S_TRIANGLE "\x81"
#define S_CIRCLE "\x82"
#define S_CROSS "\x83"

#define PRINTCHAR_MIN 0x20
#define PRINTCHAR_MAX 0x83

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

#include "psx/input.h"

#endif
