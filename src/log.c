#include <log.h>
#include <stdlib.h>

bool log_quiet = false;

bool log_msg(log_level level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (log_quiet)
        return false;

    FILE* f = stdout;
    bool should_exit = false;
    const char* log = _ERR;
    switch (level)
    {
        case LOG_INFO: log = ""; break;
        case LOG_WARN: log = _WARN; break;
        default:
            should_exit = true;
            f = stderr;
            break;
    }

    fprintf(f, "%s", log);
    vfprintf(f, fmt, args);
    fprintf(f, "\n");
    va_end(args);

    if (should_exit)
        exit(level);

    return false;
}