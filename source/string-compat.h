#ifndef STRING_COMPAT_H
#define STRING_COMPAT_H

#include <cstring>

#ifdef PSX
// #include "etl_profile.h"
// #include "etl/string.h"
// typedef etl::string<255> string;
#include "stack-string.h"
typedef stack_string<255> string;
#else
#include <string>

typedef std::string string;
#endif

#endif
