#pragma once
#include <stdarg.h>
#include <types.h>

#define _ERR "error: "
#define _WARN "warning: "
#define _INFO ""

typedef enum
{
    LOG_WARN = -1,
    LOG_INFO = 0,
    LOG_ERR,
} log_level;

extern bool log_quiet;

/// \brief Prints a log message to stdout/stderr.
/// \param level Log level.
/// \param fmt `printf` format string.
/// \returns Always `false`.
bool log_msg(log_level level, const char* fmt, ...);
