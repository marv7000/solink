#pragma once
#include <stdarg.h>
#include <types.h>

#define _ERR "error: "
#define _WARN "warning: "
#define _INFO ""

#define _RED "\x1b[31m"
#define _GREEN "\x1b[32m"
#define _YELLOW "\x1b[33m"
#define _WHITE "\x1b[37m"

#define _BOLD "\x1b[1m"
#define _REGULAR "\x1b[0m"

typedef enum
{
    LOG_WARN = -1,
    LOG_INFO = 0,
    LOG_ERR = 1,
} log_level;

extern bool log_quiet;
extern bool log_warn;

/// \brief Prints a log message to stdout/stderr.
/// \param level Log level.
/// \param fmt `printf` format string.
/// \returns Always `false`.
bool log_msg(log_level level, const str fmt, ...);
