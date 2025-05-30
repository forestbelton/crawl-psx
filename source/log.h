#ifndef LOG_H
#define LOG_H

#ifndef PSX

#define log(fmt, ...) __log(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

extern void log_init(void);

extern void __log(const char *fname, int lineno, const char *fmt, ...);

#else

#define log_init() do {} while (0)
#define log(fmt, ...) do {} while (0)

#endif

#endif
