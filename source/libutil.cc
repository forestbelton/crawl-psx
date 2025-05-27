/*
 *  File:       libutil.cc
 *  Summary:    Functions that may be missing from some systems
 *
 *  Change History (most recent first):
 *
 *      <1> 2001/Nov/01        BWR     Created
 *
 */

#include "AppHdr.h"
#include <stdio.h>
#include <ctype.h>
#include <cstring>

void get_input_line( char *const buff, int len )
{
    buff[0] = '\0';         // just in case
#ifndef PSX

#if defined(LINUX)
    get_input_line_from_curses( buff, len ); // inplemented in liblinux.cc
#elif defined(MAC) || defined(WIN32CONSOLE)
    getstr( buff, len );        // implemented in libmac.cc
#else
    fgets( buff, len, stdin );  // much safer than gets()
#endif

    buff[ len - 1 ] = '\0';  // just in case

    // Removing white space from the end in order to get rid of any
    // newlines or carriage returns that any of the above might have
    // left there (ie fgets especially).  -- bwr
    const int end = strlen( buff );
    int i;

    for (i = end - 1; i >= 0; i++)
    {
        if (isspace( buff[i] ))
            buff[i] = '\0';
        else
            break;
    }

#endif
}

// The old school way of doing short delays via low level I/O sync.
// Good for systems like old versions of Solaris that don't have usleep.
#ifdef NEED_USLEEP

#include <sys/time.h>
#include <sys/types.h>
#include <sys/unistd.h>

void usleep(unsigned long time)
{
    struct timeval timer;

    timer.tv_sec = (time / 1000000L);
    timer.tv_usec = (time % 1000000L);

    select(0, NULL, NULL, NULL, &timer);
}
#endif

// Not the greatest version of snprintf, but a functional one that's
// a bit safer than raw sprintf().  Note that this doesn't do the
// special behaviour for size == 0, largely because the return value
// in that case varies depending on which standard is being used (SUSv2
// returns an unspecified value < 1, whereas C99 allows str == NULL
// and returns the number of characters that would have been written). -- bwr
#ifdef NEED_SNPRINTF

#include <stdarg.h>
#include <string.h>

int snprintf( char *str, size_t size, const char *format, ... )
{
    va_list argp;
    va_start( argp, format );

    char buff[ 10 * size ];  // hopefully enough

    vsprintf( buff, format, argp );
    strncpy( str, buff, size );
    str[ size - 1 ] = '\0';

    int ret = strlen( str );
    if ((unsigned int) ret == size - 1 && strlen( buff ) >= size)
        ret = -1;

    va_end( argp );

    return (ret);
}
#endif

int get_random_seed()
{
#ifdef PSX
    // TODO(forest): Derive entropy from somewhere
    return 0;
#else
    return time(NULL);
#endif
}

#ifdef NEED_PERROR
void perror(const char *fmt)
{
    // TODO(forest): Write debug message directly to screen
}
#endif

/* Helper function to check if a character is whitespace */
static int is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' ||
            c == '\r' || c == '\f' || c == '\v');
}

/* Helper function to check if a character is a digit */
static int is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

static int _tolower(int c) {
    return isupper(c) ? (c - 'A' + 'a') : c;
}

#ifdef NEED_ATOI
/*
 * Convert string to integer
 * Returns the integer value represented by the string
 * Stops conversion at first non-digit character
 * Handles leading whitespace and optional sign
 *
 * Behavior matches standard atoi():
 * - Skips leading whitespace
 * - Handles optional '+' or '-' sign
 * - Converts digits until non-digit encountered
 * - Returns 0 if no valid conversion possible
 * - No overflow checking (matches standard atoi behavior)
 */
int atoi(const char *str)
{
    int result = 0;
    int sign = 1;
    const char *ptr;

    /* Handle null pointer */
    if (str == 0)
    {
        return 0;
    }

    ptr = str;

    /* Skip leading whitespace */
    while (is_whitespace(*ptr))
    {
        ptr++;
    }

    /* Handle optional sign */
    if (*ptr == '-')
    {
        sign = -1;
        ptr++;
    }
    else if (*ptr == '+')
    {
        ptr++;
    }

    /* Convert digits */
    while (is_digit(*ptr))
    {
        result = result * 10 + (*ptr - '0');
        ptr++;
    }

    return sign * result;
}
#endif

#ifdef NEED_ATOL
/*
 * Convert string to integer
 * Returns the integer value represented by the string
 * Stops conversion at first non-digit character
 * Handles leading whitespace and optional sign
 *
 * Behavior matches standard atoi():
 * - Skips leading whitespace
 * - Handles optional '+' or '-' sign
 * - Converts digits until non-digit encountered
 * - Returns 0 if no valid conversion possible
 * - No overflow checking (matches standard atoi behavior)
 */
long atol(const char *str)
{
    long result = 0;
    long sign = 1;
    const char *ptr;

    /* Handle null pointer */
    if (str == 0)
    {
        return 0;
    }

    ptr = str;

    /* Skip leading whitespace */
    while (is_whitespace(*ptr))
    {
        ptr++;
    }

    /* Handle optional sign */
    if (*ptr == '-')
    {
        sign = -1;
        ptr++;
    }
    else if (*ptr == '+')
    {
        ptr++;
    }

    /* Convert digits */
    while (is_digit(*ptr))
    {
        result = result * 10 + (*ptr - '0');
        ptr++;
    }

    return sign * result;
}
#endif

#ifdef NEED_ISALNUM
int isalnum(int c) {
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z');
}
#endif

#ifdef NEED_ISUPPER
int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}
#endif

#ifdef NEED_ITOA
char* itoa(int value, char* str, int base) {
    // Handle invalid base
    if (base < 2 || base > 36) {
        str[0] = '\0';
        return str;
    }
    
    char* ptr = str;
    char* start = str;
    int is_negative = 0;
    
    // Handle negative numbers (only for base 10)
    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
        *ptr++ = '-';
        start = ptr; // Start of digits after the minus sign
    }
    
    // Handle zero case
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }
    
    // Convert integer to string (digits will be in reverse order)
    while (value > 0) {
        int digit = value % base;
        if (digit < 10) {
            *ptr++ = '0' + digit;
        } else {
            *ptr++ = 'A' + (digit - 10);
        }
        value /= base;
    }
    
    // Null terminate
    *ptr = '\0';
    
    // Reverse the digits (excluding the minus sign if present)
    char* end = ptr - 1;
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}
#endif

#ifdef NEED_STRICMP
int stricmp(const char *s, const char *t) {
    int diff = 0;
    while (*s && *t && _tolower(*s) == _tolower(*t)) {
        s++;
        t++;
    }
    return _tolower(*s) - _tolower(*t);
}
#endif

#ifdef NEED_STRLWR
char *strlwr(char *s) {
    while (*s) {
        *s = _tolower(*s);
        s++;
    }
}
#endif
