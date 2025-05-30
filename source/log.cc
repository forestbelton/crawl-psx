#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static FILE *LOGFILE;

void log_init() {
    LOGFILE = fopen("crawl.log", "a+");
}

void __log(const char *fname, int lineno, const char *fmt, ...) {
    // Get current time and format it
    time_t now;
    struct tm *timeinfo;
    char timestamp[32];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Print timestamp first
    fprintf(LOGFILE, "[%s] [%s:%d] ", timestamp, fname, lineno);
    
    // Print the actual log message
    va_list args;
    va_start(args, fmt);
    vfprintf(LOGFILE, fmt, args);
    va_end(args);
    
    fflush(LOGFILE);
}