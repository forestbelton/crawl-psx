/*
 *  File:       libutil.h
 *  Summary:    System indepentant functions
 *
 *  Change History (most recent first):
 *
 *      <1>   2001/Nov/01        BWR     Created
 *
 */

#ifndef LIBUTIL_H
#define LIBUTIL_H

void get_input_line( char *const buff, int len );

#ifdef NEED_USLEEP
void usleep( unsigned long time );
#endif

#ifdef NEED_SNPRINTF
int snprintf( char *str, size_t size, const char *format, ... );
#endif

#ifdef NEED_PERROR
void perror(const char *fmt);
#endif

#ifdef NEED_ATOI
int atoi(const char *s);
#endif

#ifdef NEED_ATOL
long atol(const char *s);
#endif

#ifdef NEED_ISALNUM
int isalnum(int c);
#endif

#ifdef NEED_ISDIGIT
int isdigit(int c);
#endif

#ifdef NEED_ISSPACE
int isspace(int c);
#endif

#ifdef NEED_ISUPPER
int isupper(int c);
#endif

#ifdef NEED_ITOA
char* itoa(int value, char* str, int base);
#endif

#ifdef NEED_STRICMP
int stricmp(const char *s, const char *t);
#endif

#ifdef NEED_STRLWR
char *strlwr(char *s);
#endif

#ifdef NEED_TOLOWER
int tolower(int c);
#endif

int get_random_seed(void);

#endif
