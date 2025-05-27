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

int get_random_seed(void);

#endif
