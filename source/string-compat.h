#ifndef STRING_COMPAT_H
#define STRING_COMPAT_H

#include <cstring>

#ifdef PSX
#include "stack-string.h"
typedef stack_string<500> string;
#else
#include <string>

typedef std::string string;
#endif

#endif
